#include "FU.h"

extern config *CPU_cfg;
extern clk_tick sys_clk;
extern FU_CDB fCDB;
extern vector<int*> clk_wait_list;
extern ROB *CPU_ROB;
extern memory main_mem;
extern vector<intAdder*> iAdder;
extern vector<flpAdder*> fAdder;
extern vector<flpMtplr*> fMtplr;
extern vector<ldsdUnit*> lsUnit;

FU_CDB::FU_CDB()
{
    next_vdd = 0;
    front = rear = 0;
    bus.source = -1;
    Q_lock = PTHREAD_MUTEX_INITIALIZER;
}

bool FU_CDB::enQ(valType t, void *v, int s)
{
    pthread_mutex_lock(&Q_lock);
    if ((rear+1)%Q_LEN == front)
    {
        err_log("The reserv_CDB queue is full");
        pthread_mutex_unlock(&Q_lock);
        return false;
    }
    int index = rear;
    rear = (++rear)%Q_LEN;
    int pos = 0;
    for (; (front + pos)%Q_LEN != index && queue[(front + pos)%Q_LEN].source < s; pos++);
    for (int i = index; (Q_LEN + i)%Q_LEN != (front + pos)%Q_LEN; i--)
    {
        queue[(Q_LEN+i)%Q_LEN].source = queue[(Q_LEN+i-1)%Q_LEN].source;
        queue[(Q_LEN+i)%Q_LEN].value = queue[(Q_LEN+i-1)%Q_LEN].value;
        queue[(Q_LEN+i)%Q_LEN].type = queue[(Q_LEN+i-1)%Q_LEN].type;
    }
    queue[(front+pos)%Q_LEN].type = t;
    if (t == INTGR)
        queue[(front+pos)%Q_LEN].value.i = *(int*)v;
    else
        queue[(front+pos)%Q_LEN].value.f = *(float*)v;
    queue[(front+pos)%Q_LEN].source = s;
    pthread_mutex_unlock(&Q_lock);
    return true;
}

int FU_CDB::get_source()
{
    return bus.source;
}

bool FU_CDB::get_val(void *v)
{
    if (bus.type == INTGR)
        *(int*)v = bus.value.i;
    else
        *(float*)v = bus.value.f;
    return true;
}

void FU_CDB::CDB_automat()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while(true)
    {
        at_rising_edge(&lock, next_vdd);
        at_falling_edge(&lock, next_vdd);
        if (front != rear)
        {
            bus.source = queue[front].source;
            bus.type = queue[front].type;
            bus.value = queue[front].value;
            msg_log("About to broadcast, source = ROB " + to_string(bus.source), 3);
            front = (++front)%Q_LEN;
        }
    }
}

void init_FU_CDB()
{
    clk_wait_list.push_back(&fCDB.next_vdd);
    pthread_create(&fCDB.handle, NULL, [](void *arg)->void*{fCDB.CDB_automat(); return NULL;}, NULL);
}

FU_Q::FU_Q()
{
    front = rear = 0;
    Q_lock = PTHREAD_MUTEX_INITIALIZER;
}

const FU_QEntry* FU_Q::enQ(opCode c, valType rt, int d, memCell *r, memCell *op1, memCell *op2, int offst)
{
    pthread_mutex_lock(&Q_lock);
    if ((rear+1)%Q_LEN == front)
    {
        pthread_mutex_unlock(&Q_lock);
        throw -1;
    }
    int index = rear;
    rear = (++rear)%Q_LEN;
    queue[index].code = c;
    queue[index].rtType = rt;
    queue[index].dest = d;
    queue[index].res = r;
    queue[index].oprnd1 = op1;
    queue[index].oprnd2 = op2;
    queue[index].offset = offst;
    queue[index].done = false;
    pthread_mutex_unlock(&Q_lock);
    return &queue[index];
}

FU_QEntry *FU_Q::deQ()
{
    if (front == rear)
        return nullptr;
    FU_QEntry* ret = &queue[front];
    front = (++front)%Q_LEN;
    return ret;
}

functionUnit::functionUnit()
{
    next_vdd = 0;
    task.done = true;
}

intAdder::intAdder()
{
    next_vdd = 0;
    task.done = true;
    for (int i = 0; i<CPU_cfg->int_add->rs_num; i++)
    {
        auto tmp = new resStation(&queue, INTGR);
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &rs_thread_container, tmp);
        rs.push_back(tmp);
    }
}

intAdder::~intAdder()
{
    for (auto &p : rs)
    {
        delete p;
        p = nullptr;
    }
}

void intAdder::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (task.done)
        {
            auto newTask = queue.deQ();
            if (newTask != nullptr)
                task = *newTask;
        }
        if (!task.done)
        {
            ROBEntry *R = CPU_ROB->get_entry(task.dest);
            R->output.exe = sys_clk.get_prog_cyc();
            for (int i = 0; ;i++)
            {
                msg_log("Calculating INT "+to_string(task.oprnd1->i)+" + "+to_string(task.oprnd2->i), 3);
                if (i == CPU_cfg->int_add->exe_time - 1)
                {
                    task.res->i = task.oprnd1->i + task.oprnd2->i;
                    fCDB.enQ(INTGR, task.res, task.dest);
                    task.done = true;
                    break;
                }
                at_falling_edge(&lock, next_vdd);
                at_rising_edge(&lock, next_vdd);
            }
        }
        at_falling_edge(&lock, next_vdd);
    }
}

flpAdder::flpAdder()
{
    next_vdd = 0;
    task.done = true;
    for (int i = 0; i<CPU_cfg->fp_add->rs_num; i++)
    {
        auto tmp = new resStation(&queue, FLTP);
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &rs_thread_container, tmp);
        rs.push_back(tmp);
    }
    int latency = CPU_cfg->fp_add->exe_time - 1;
    pLatency_Q = nullptr;
    if (latency)
    {
        pLatency_Q = new FU_QEntry[latency];
        for (int i = 0; i<latency; i++)
            pLatency_Q[i].res = nullptr;
    }
}

flpAdder::~flpAdder()
{
    for (auto &p : rs)
    {
        delete p;
        p = nullptr;
    }
    if (pLatency_Q)
        delete pLatency_Q;
}

void flpAdder::shift(FU_QEntry &in, FU_QEntry &out)
{
    int size = CPU_cfg->fp_add->exe_time - 1;
    if (!size)
    {
        out = in;
        return;
    }
    out = pLatency_Q[size-1];
    for (int i = size-1; i>0; i--)
        pLatency_Q[i] = pLatency_Q[i-1];
    pLatency_Q[0] = in;
}

void flpAdder::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    FU_QEntry shift_out;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        task.res = nullptr;
        auto newTask = queue.deQ();
        if (newTask != nullptr)
        {
            task = *newTask;
            ROBEntry *R = CPU_ROB->get_entry(task.dest);
            R->output.exe = sys_clk.get_prog_cyc();
            msg_log("Calculating FLP "+to_string(task.oprnd1->f)+" + "+to_string(task.oprnd2->f), 3);
            task.res->f = task.oprnd1->f + task.oprnd2->f;
        }
        shift(task, shift_out);
        if (shift_out.res)
            fCDB.enQ(FLTP, shift_out.res, shift_out.dest);
        at_falling_edge(&lock, next_vdd);
    }
}

flpMtplr::flpMtplr()
{
    next_vdd = 0;
    task.done = true;
    for (int i = 0; i<CPU_cfg->fp_mul->rs_num; i++)
    {
        auto tmp = new resStation(&queue, FLTP);
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &rs_thread_container, tmp);
        rs.push_back(tmp);
    }
    int latency = CPU_cfg->fp_mul->exe_time - 1;
    pLatency_Q = nullptr;
    if (latency)
    {
        pLatency_Q = new FU_QEntry[latency];
        for (int i = 0; i<latency; i++)
            pLatency_Q[i].res = nullptr;
    }
}

flpMtplr::~flpMtplr()
{
    for (auto &p : rs)
    {
        delete p;
        p = nullptr;
    }
    if (pLatency_Q)
        delete pLatency_Q;
}

void flpMtplr::shift(FU_QEntry &in, FU_QEntry &out)
{
    int size = CPU_cfg->fp_mul->exe_time - 1;
    if (!size)
    {
        out = in;
        return;
    }
    out = pLatency_Q[size-1];
    for (int i = size-1; i>0; i--)
        pLatency_Q[i] = pLatency_Q[i-1];
    pLatency_Q[0] = in;
}

void flpMtplr::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    FU_QEntry shift_out;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        task.res = nullptr;
        auto newTask = queue.deQ();
        if (newTask != nullptr)
        {
            task = *newTask;
            ROBEntry *R = CPU_ROB->get_entry(task.dest);
            R->output.exe = sys_clk.get_prog_cyc();
            msg_log("Calculating FLP "+to_string(task.oprnd1->f)+" X "+to_string(task.oprnd2->f), 3);
            task.res->f = task.oprnd1->f * task.oprnd2->f;
        }
        shift(task, shift_out);
        if (shift_out.res)
            fCDB.enQ(FLTP, shift_out.res, shift_out.dest);
        at_falling_edge(&lock, next_vdd);
    }
}

ldsdUnit::ldsdUnit()
{
    next_vdd = 0;
    task.done = true;
    for (int i = 0; i<CPU_cfg->ld_str->rs_num; i++)
    {
        auto tmp = new resStation(&queue, INTGR);
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &rs_thread_container, tmp);
        rs.push_back(tmp);
    }
}

ldsdUnit::~ldsdUnit()
{
    for (auto &p : rs)
    {
        delete p;
        p = nullptr;
    }
}

void ldsdUnit::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (task.done)
        {
            auto newTask = queue.deQ();
            if (newTask != nullptr)
                task = *newTask;
        }
        if (!task.done)
        {
            ROBEntry *R = CPU_ROB->get_entry(task.dest);
            R->output.exe = sys_clk.get_prog_cyc();
            for (int i = 0; ;i++)
            {
                msg_log("Calculating address "+to_string(*(int*)task.oprnd1)+" + "+to_string(task.offset), 3);
                if (i == CPU_cfg->ld_str->exe_time - 1)
                {
                    int addr = *(int*)task.oprnd1 + task.offset;
                    at_falling_edge(&lock, next_vdd);
                    try {main_mem.enQ(task.dest, task.code, task.rtType, addr, task.oprnd2);}
                    catch(const int e){err_log("Exception = "+to_string(e));}
                    task.done = true;
                    break;
                }
                at_falling_edge(&lock, next_vdd);
                at_rising_edge(&lock, next_vdd);
            }
        }
        at_falling_edge(&lock, next_vdd);
    }
}

void* intAdder_thread_container(void *arg)
{
    auto p = (intAdder*) arg;
    p->FU_automate();
    return nullptr;
}

void* flpAdder_thread_container(void *arg)
{
    auto p = (flpAdder*) arg;
    p->FU_automate();
    return nullptr;
}

void* flpMlptr_thread_container(void *arg)
{
    auto p = (flpMtplr*) arg;
    p->FU_automate();
    return nullptr;
}

void* ldsdUnit_thread_container(void *arg)
{
    auto p = (ldsdUnit*) arg;
    p->FU_automate();
    return nullptr;
}

void init_FUs()
{
    if (iAdder.size())
        for (auto &p : iAdder)
            delete p;
    if (fAdder.size())
        for (auto &p : fAdder)
            delete p;
    if (fMtplr.size())
        for (auto &p : fMtplr)
            delete p;
    if (lsUnit.size())
        for (auto &p : lsUnit)
            delete p;
    iAdder = {};
    fAdder = {};
    fMtplr = {};
    lsUnit = {};
    for (int i = 0; i<CPU_cfg->int_add->fu_num; i++)
    {
        auto tmp = new intAdder();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &intAdder_thread_container, tmp);
        iAdder.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->fp_add->fu_num; i++)
    {
        auto tmp = new flpAdder();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &flpAdder_thread_container, tmp);
        fAdder.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->fp_mul->fu_num; i++)
    {
        auto tmp = new flpMtplr();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &flpMlptr_thread_container, tmp);
        fMtplr.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->ld_str->fu_num; i++)
    {
        auto tmp = new ldsdUnit();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &ldsdUnit_thread_container, tmp);
        lsUnit.push_back(tmp);
    }
}