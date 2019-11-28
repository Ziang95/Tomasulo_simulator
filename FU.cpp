#include "FU.h"

extern config *CPU_cfg;
extern instr_queue *instr_Q;
extern clk_tick sys_clk;
extern FU_CDB fCDB;
extern vector<int*> clk_wait_list;
extern ROB *CPU_ROB;
extern BTB CPU_BTB;
extern branchCtrl brcUnit;
extern memory main_mem;
extern vector<intAdder*> iAdder;
extern vector<flpAdder*> fAdder;
extern vector<flpMtplr*> fMtplr;
extern vector<ldsdUnit*> lsUnit;
extern nopBublr* nopBub;

FU_CDB::FU_CDB()
{
    front = rear = 0;
    bus.source = -2;
    Q_lock = PTHREAD_MUTEX_INITIALIZER;
}

bool FU_CDB::enQ(memCell *v, int s)
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
        queue[(Q_LEN+i)%Q_LEN] = queue[(Q_LEN+i-1)%Q_LEN];
    index = (front+pos)%Q_LEN;
    queue[index].value = *v;
    queue[index].source = s;
    pthread_mutex_unlock(&Q_lock);
    return true;
}

int FU_CDB::get_source()
{
    return bus.source;
}

bool FU_CDB::get_val(memCell *v)
{
    *v = bus.value;
    return true;
}

void FU_CDB::squash(int ROB_i)
{
    int R_f = CPU_ROB->get_front();
    int R_r = CPU_ROB->get_rear();
    pthread_mutex_lock(&Q_lock);
    if (front == rear)
    {
        pthread_mutex_unlock(&Q_lock);
        return;
    }
    for (int i = rear; ;)
    {
        int j = (i-1)%Q_LEN;
        if (j == front || !is_prev_index(ROB_i, queue[j].source, R_f, R_r))
            break;
        rear = j;
        i = (--i)%Q_LEN;
    }
    if (is_prev_index(ROB_i, bus.source, R_f, R_r))
        bus.source = -2;
    pthread_mutex_unlock(&Q_lock);
}

void FU_CDB::CDB_automat()
{
    next_vdd = 0;
    while(true)
    {
        at_rising_edge(next_vdd);
        at_falling_edge(next_vdd);
        pthread_mutex_lock(&Q_lock);
        if (front != rear)
        {
            bus.source = queue[front].source;
            bus.value = queue[front].value;
            msg_log("About to broadcast, source ROB = " + to_string(bus.source), 3);
            front = (++front)%Q_LEN;
        }
        pthread_mutex_unlock(&Q_lock);
    }
}

void init_FU_CDB()
{
    int ret = 1;
    while(ret) ret = pthread_create(&fCDB.handle, NULL, [](void *arg)->void*{fCDB.CDB_automat(); return NULL;}, NULL);
    clk_wait_list.push_back(&fCDB.next_vdd);
}

FU_Q::FU_Q()
{
    front = rear = 0;
    Q_lock = PTHREAD_MUTEX_INITIALIZER;
}

const FU_QEntry* FU_Q::enQ(opCode c, int d, memCell *r, memCell *op1, memCell *op2, LSQEntry *_lsqe, int offst, bool* busy)
{
    pthread_mutex_lock(&Q_lock);
    if ((rear+1)%Q_LEN == front)
    {
        pthread_mutex_unlock(&Q_lock);
        throw -1;
    }
    int index = rear;
    rear = (++rear)%Q_LEN;
    int pos = 0;
    for (; (front + pos)%Q_LEN != index && queue[(front + pos)%Q_LEN].dest < d; pos++);
    for (int i = index; (Q_LEN + i)%Q_LEN != (front + pos)%Q_LEN; i--)
        queue[(Q_LEN+i)%Q_LEN] = queue[(Q_LEN+i-1)%Q_LEN];
    index = (front+pos)%Q_LEN;
    queue[index].code = c;
    queue[index].dest = d;
    queue[index].res = r;
    queue[index].oprnd1 = op1;
    queue[index].oprnd2 = op2;
    queue[index].offset = offst;
    queue[index].lsqe = _lsqe;
    queue[index].busy = busy;
    pthread_mutex_unlock(&Q_lock);
    return &queue[index];
}

void FU_Q::squash(int ROB_i)
{
    int R_f = CPU_ROB->get_front();
    int R_r = CPU_ROB->get_rear();
    pthread_mutex_lock(&Q_lock);
    if (front == rear)
    {
        pthread_mutex_unlock(&Q_lock);
        return;
    }
    for (int i = rear; ;)
    {
        int j = (i-1)%Q_LEN;
        if (j == front || !is_prev_index(ROB_i, queue[j].dest, R_f, R_r))
            break;
        rear = j;
        i = (--i)%Q_LEN;
    }
    pthread_mutex_unlock(&Q_lock);
}

FU_QEntry *FU_Q::deQ()
{
    if (front == rear)
        return nullptr;
    FU_QEntry* ret = &queue[front];
    front = (++front)%Q_LEN;
    return ret;
}

void functionUnit::squash(int ROB_i)
{
    for (auto _rs : rs)
        _rs->squash(ROB_i);
    queue.squash(ROB_i);
    squash_p.R_f = CPU_ROB->get_front();
    squash_p.R_r = CPU_ROB->get_rear();
    squash_p.ROB_i = brcUnit.squash_ROB_i();
    squash_p.flag = true;
}

intAdder::intAdder()
{
    for (int i = 0; i<CPU_cfg->int_add->rs_num; i++)
    {
        auto tmp = new resStation(&queue, INTGR);
        init_resStation(tmp);
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

void intAdder::FU_automat()
{
    next_vdd = 0;
    while (true)
    {
A:      at_rising_edge(next_vdd);
        if (squash_p.flag)
        {
            squash_p.flag = false;
            at_falling_edge(next_vdd);
            continue;
        }
        auto newTask = queue.deQ();
        if (newTask)
        {
            task = *newTask;
            ROBEntry *R = CPU_ROB->get_entry(task.dest);
            R->output.exe = sys_clk.get_prog_cyc();
            for (int i = 0; ;i++)
            {
                if (squash_p.flag)
                {
                    squash_p.flag = false;
                    if (is_prev_index(squash_p.ROB_i, task.dest, squash_p.R_f, squash_p.R_r))
                        break;
                }
                msg_log("Calculating INT "+to_string(task.oprnd1->i)+" + "+to_string(task.oprnd2->i) + " dest ROB = " + to_string(task.dest), 3);
                if (i == CPU_cfg->int_add->exe_time - 1)
                {
                    if (task.code == BEQ || task.code == BNE)
                    {
                        *task.busy = false;
                        bool branch = false;
                        int result = task.oprnd1->i - task.oprnd2->i;
                        if (result)
                            branch = task.code == BNE? true:false;
                        else
                            branch = task.code == BEQ? true:false;
                        if (auto tmp = CPU_BTB.getEntry(R->instr_i))
                        {
                            if (branch == tmp->taken)
                            {
                                at_falling_edge(next_vdd);
                                R->finished = true;
                            }
                            else
                            {
                                tmp->taken = branch;
                                if (branch)
                                    brcUnit.to_target(tmp->target);
                                else
                                    brcUnit.to_target(R->instr_i + 1);
                                msg_log("Begin Squash", 3);
                                brcUnit.to_squash(task.dest);
                                at_falling_edge(next_vdd);
                            }
                        }
                        else
                        {
                            if (branch)
                            {
                                CPU_BTB.addEntry(R->instr_i, (R->instr_i + 1 + task.offset)%8);
                                brcUnit.to_target(R->instr_i + 1 + task.offset);
                                msg_log("Begin Squash", 3);
                                brcUnit.to_squash(task.dest);
                                at_falling_edge(next_vdd);
                            }
                            else
                            {
                                at_falling_edge(next_vdd);
                                R->finished = true;
                            }
                        }
                        goto A;
                    }
                    else
                    {
                        task.res->i = task.oprnd1->i + task.oprnd2->i;
                        msg_log("CDB enqueued, ROB = " + to_string(task.dest), 3);
                        fCDB.enQ(task.res, task.dest);
                    }
                    break;
                }
                at_falling_edge(next_vdd);
                at_rising_edge(next_vdd);
            }
        }
        at_falling_edge(next_vdd);
    }
}

flpAdder::flpAdder()
{
    for (int i = 0; i<CPU_cfg->fp_add->rs_num; i++)
    {
        auto tmp = new resStation(&queue, FLTP);
        init_resStation(tmp);
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

void flpAdder::FU_automat()
{
    next_vdd = 0;
    FU_QEntry shift_out;
    while (true)
    {
        at_rising_edge(next_vdd);
        if (squash_p.flag)
        {
            for (int i = 0; i<CPU_cfg->fp_add->exe_time - 1; i++)
                if (pLatency_Q[i].res && is_prev_index(squash_p.ROB_i, pLatency_Q[i].dest, squash_p.R_f, squash_p.R_r))
                    pLatency_Q[i].res = nullptr;
            squash_p.flag = false;
        }
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
            fCDB.enQ(shift_out.res, shift_out.dest);
        at_falling_edge(next_vdd);
    }
}

flpMtplr::flpMtplr()
{
    for (int i = 0; i<CPU_cfg->fp_mul->rs_num; i++)
    {
        auto tmp = new resStation(&queue, FLTP);
        init_resStation(tmp);
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

void flpMtplr::FU_automat()
{
    next_vdd = 0;
    FU_QEntry shift_out;
    while (true)
    {
        at_rising_edge(next_vdd);
        if (squash_p.flag)
        {
            for (int i = 0; i<CPU_cfg->fp_mul->exe_time - 1; i++)
                if (pLatency_Q[i].res && is_prev_index(squash_p.ROB_i, pLatency_Q[i].dest, squash_p.R_f, squash_p.R_r))
                    pLatency_Q[i].res = nullptr;
            squash_p.flag = false;
        }
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
            fCDB.enQ(shift_out.res, shift_out.dest);
        at_falling_edge(next_vdd);
    }
}

ldsdUnit::ldsdUnit()
{
    for (int i = 0; i<CPU_cfg->ld_str->rs_num; i++)
    {
        auto tmp = new resStation(&queue, INTGR);
        init_resStation(tmp);
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

void ldsdUnit::FU_automat()
{
    next_vdd = 0;
    while (true)
    {
A:      at_rising_edge(next_vdd);
        if (squash_p.flag)
        {
            squash_p.flag = false;
            at_falling_edge(next_vdd);
            continue;
        }
        auto newTask = queue.deQ();
        if (newTask)
        {
            task = *newTask;
            ROBEntry *R = CPU_ROB->get_entry(task.dest);
            R->output.exe = sys_clk.get_prog_cyc();
            for (int i = 0; ;i++)
            {
                if (squash_p.flag)
                {
                    squash_p.flag = false;
                    if (is_prev_index(squash_p.ROB_i, task.dest, squash_p.R_f, squash_p.R_r))
                        break;
                }
                msg_log("Calculating address "+to_string(task.oprnd1->i)+" + "+to_string(task.offset) + ", ROB = " + to_string(task.dest), 3);
                if (i == CPU_cfg->ld_str->exe_time - 1)
                {
                    task.lsqe->addr = task.oprnd1->i + task.offset;
                    if (task.code == SD)
                    {
                        *task.busy = false;
                        if (task.lsqe->SD_source == -1)
                        {
                            at_falling_edge(next_vdd);
                            task.lsqe->ready = true;
                            R->finished = true;
                            goto A;
                        }
                    }
                    else
                    {
                        at_falling_edge(next_vdd);
                        task.lsqe->ready = true;
                        goto A;
                    }
                    break;
                }
                at_falling_edge(next_vdd);
                at_rising_edge(next_vdd);
            }
        }
        at_falling_edge(next_vdd);
    }
}

nopBublr::nopBublr()
{
    bubble.ROB_i = -1;
    bubble.ROB_E = nullptr;
    for (int i = 0; i<3; i++)
    {
        shifter[i].ROB_i = -1;
        shifter[i].ROB_E = nullptr;
    }
}

void nopBublr::generate_bubble(int ROB_i)
{
    bubble.ROB_i = ROB_i;
    bubble.ROB_E = CPU_ROB->get_entry(ROB_i);
}

void nopBublr::squash(int ROB_i)
{
    for (auto &s : shifter)
        if (is_prev_index(ROB_i, s.ROB_i, CPU_ROB->get_front(), CPU_ROB->get_rear()))
            s.ROB_E = nullptr;
}

void nopBublr::FU_automat()
{
    next_vdd = 0;
    while (true)
    {
        at_rising_edge(next_vdd);
        for (int i = 2; i>0; i--)
            shifter[i] = shifter[i-1];
        shifter[0] = bubble;
        bubble.ROB_i = -1;
        bubble.ROB_E = nullptr;
        if (shifter[0].ROB_E)
            shifter[0].ROB_E->output.exe = sys_clk.get_prog_cyc();
        if (shifter[1].ROB_E)
        {
            shifter[1].ROB_E->output.wBack = sys_clk.get_prog_cyc();
            shifter[1].ROB_E->wrtnBack = true;
        }
        if (shifter[2].ROB_E)
        {
            shifter[2].ROB_E->output.commit = sys_clk.get_prog_cyc();
            shifter[2].ROB_E->finished = true;
        }
        at_falling_edge(next_vdd);
    }
}

void* intAdder_thread_container(void *arg)
{
    auto p = (intAdder*) arg;
    p->FU_automat();
    return nullptr;
}

void* flpAdder_thread_container(void *arg)
{
    auto p = (flpAdder*) arg;
    p->FU_automat();
    return nullptr;
}

void* flpMlptr_thread_container(void *arg)
{
    auto p = (flpMtplr*) arg;
    p->FU_automat();
    return nullptr;
}

void* ldsdUnit_thread_container(void *arg)
{
    auto p = (ldsdUnit*) arg;
    p->FU_automat();
    return nullptr;
}

void* nopBublr_thread_container(void *arg)
{
    auto p = (nopBublr*) arg;
    p->FU_automat();
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
    if (nopBub)
        delete nopBub;
    iAdder = {};
    fAdder = {};
    fMtplr = {};
    lsUnit = {};
    for (int i = 0; i<CPU_cfg->int_add->fu_num; i++)
    {
        auto tmp = new intAdder();
        while(pthread_create(&tmp->handle, NULL, &intAdder_thread_container, tmp));
        clk_wait_list.push_back(&tmp->next_vdd);
        iAdder.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->fp_add->fu_num; i++)
    {
        auto tmp = new flpAdder();
        while(pthread_create(&tmp->handle, NULL, &flpAdder_thread_container, tmp));
        clk_wait_list.push_back(&tmp->next_vdd);
        fAdder.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->fp_mul->fu_num; i++)
    {
        auto tmp = new flpMtplr();
        while(pthread_create(&tmp->handle, NULL, &flpMlptr_thread_container, tmp));
        clk_wait_list.push_back(&tmp->next_vdd);
        fMtplr.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->ld_str->fu_num; i++)
    {
        auto tmp = new ldsdUnit();
        while(pthread_create(&tmp->handle, NULL, &ldsdUnit_thread_container, tmp));
        clk_wait_list.push_back(&tmp->next_vdd);
        lsUnit.push_back(tmp);
    }
    nopBub = new nopBublr();
    while(pthread_create(&nopBub->handle, NULL, &nopBublr_thread_container, nopBub));
    clk_wait_list.push_back(&nopBub->next_vdd);
}