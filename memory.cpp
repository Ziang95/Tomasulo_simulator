#include "memory.h"

extern clk_tick sys_clk;
extern memory main_mem;
extern config *CPU_cfg;
extern FU_CDB fCDB;
extern vector<int*> clk_wait_list;
extern instr_queue *instr_Q;
extern branchCtrl brcUnit;
extern ROB *CPU_ROB;

memory::memory(int sz)
{
    buf = new memCell[sz];
    for (int i = 0; i < sz; i++)
        buf[i].f = 0.;
    size = sz;
    Lfront = Lrear = 0;
    Sfront = Srear = 0;
    LDQ_lock = SDQ_lock = PTHREAD_MUTEX_INITIALIZER;
}

memory::~memory()
{
    delete []buf;
}

bool memory::store(LSQEntry& entry)
{
    ROBEntry *R = CPU_ROB->get_entry(entry.rob_i);
    R->output.commit = R->output.mem = sys_clk.get_prog_cyc();
    instr_timeline_output(R);
    msg_log("Begin to store value to Addr="+to_string(entry.addr), 3);
    for (int i = 0; ; i++)
    {
        msg_log("Storing value to Addr="+to_string(entry.addr), 3);
        if (i == CPU_cfg->ld_str->mem_time - 1)
            break;
        at_falling_edge(mem_next_vdd);
        if (i == 0)
            CPU_ROB->ptr_advance();
        at_rising_edge(mem_next_vdd);
    }
    buf[entry.addr] = entry.val;
    at_falling_edge(mem_next_vdd);
    if (CPU_cfg->ld_str->mem_time == 1) //In this case the ROB pointer was not advanced in the loop
        CPU_ROB->ptr_advance();
    Sfront = (++Sfront)%Q_LEN;
    msg_log("Value stored ROB = " + to_string(entry.rob_i), 2);
    return true;
}

bool memory::load(LSQEntry& entry)
{
    ROBEntry *R = CPU_ROB->get_entry(entry.rob_i);
    int found = -1;
    bool no_blocking_SD = true;
    memCell ret;
    for (int i = Srear; is_prev_index(Sfront, i, Sfront, Srear);)
    {
        i = (--i)%Q_LEN;
        if (is_prev_index(Stor_Q[i].rob_i, entry.rob_i, CPU_ROB->get_front(), CPU_ROB->get_rear()))
        {
            if (Stor_Q[i].addr == -1)
            {
                no_blocking_SD = false;
                break;
            }
            else
            {
                if (Stor_Q[i].addr == entry.addr && found == -1)
                {
                    if (Stor_Q[i].ready)
                        found = i;
                    else
                    {
                        no_blocking_SD = false;
                        break;
                    }
                }
            }
        }
    }
    if (no_blocking_SD)
    {
        R->output.mem = sys_clk.get_prog_cyc();
        if (found >= 0)
        {
            msg_log("Forward store found, forwarding, Addr="+to_string(Stor_Q[found].addr), 3);
            ret = Stor_Q[found].val;
        }
        else
        {
            msg_log("Begin LD, ROB = " + to_string(entry.rob_i), 3);
            for (int i = 0; ; i++)
            {
                // if (instr_Q->squash && is_prev_index(brcUnit.squash_ROB_i(), entry.rob_i, CPU_ROB->get_front(), CPU_ROB->get_rear()))
                // {
                //     at_falling_edge(mem_next_vdd);
                //     return true;
                // }
                if (i == CPU_cfg->ld_str->mem_time - 1)
                {
                    ret = buf[entry.addr];
                    break;
                }
                at_falling_edge(mem_next_vdd);
                at_rising_edge(mem_next_vdd);
                msg_log("Loading value from Addr="+to_string(entry.addr), 3);
            }
        }
        fCDB.enQ(&ret, entry.rob_i);
        at_falling_edge(mem_next_vdd);
        Lfront = (++Lfront)%Q_LEN;
        msg_log("Value loaded ROB = " + to_string(entry.rob_i), 2);
    }
    else
    {
        msg_log("Previous SD not ready, caller ROB = " + to_string(Load_Q[Lfront].rob_i), 3);
        at_falling_edge(mem_next_vdd);
    }
    return true;
}

LSQEntry* memory::LD_enQ(int rbi, int addr)
{
    pthread_mutex_lock(&LDQ_lock);
    if ((Lrear+1)%Q_LEN == Lfront)
    {
        pthread_mutex_unlock(&LDQ_lock);
        throw -2;
    }
    int index = Lrear;
    Lrear = (++Lrear)%Q_LEN;
    Load_Q[index].rob_i = rbi;
    Load_Q[index].addr = addr;
    Load_Q[index].code = LD;
    Load_Q[index].ready = false;
    pthread_mutex_unlock(&LDQ_lock);
    return Load_Q + index;
}

LSQEntry* memory::SD_enQ(int rbi, int Qj, int addr, memCell val)
{
    pthread_mutex_lock(&SDQ_lock);
    if ((Srear+1)%Q_LEN == Sfront)
    {
        pthread_mutex_unlock(&SDQ_lock);
        throw -1;
    }
    int index = Srear;
    Srear = (++Srear)%Q_LEN;
    Stor_Q[index].rob_i = rbi;
    Stor_Q[index].addr = addr;
    Stor_Q[index].SD_source = Qj;
    Stor_Q[index].code = LD;
    Stor_Q[index].val = val;
    Stor_Q[index].ready = false;
    pthread_mutex_unlock(&SDQ_lock);
    return Stor_Q + index;
}

bool memory::setMem(valType type, int addr, void* val)
{
    if (addr>=size){
        err_log("Mem set out of range");
        return false;
    }
    if (type == INTGR)
        buf[addr].i = *(int*)val;
    else
        buf[addr].f = *(float*)val;
    return true;
}

memCell memory::getMem(int addr)
{
    return buf[addr];
}

bool is_prev_index(int i, int j, int front, int rear)
{
    if (i == j) return false;
    if (rear < front)
    {
        rear += CPU_ROB->size;
        i += i<front?CPU_ROB->size:0;
        j += j<front?CPU_ROB->size:0;
    }
    return j - i > 0;
}

void memory::squash(int ROB_i)
{
    int R_f = CPU_ROB->get_front();
    int R_r = CPU_ROB->get_rear();
    pthread_mutex_lock(&LDQ_lock);
    for (int i = Lrear; ;)
    {
        int j = (i-1)%Q_LEN;
        if (j == Lfront || !is_prev_index(ROB_i, Load_Q[j].rob_i, R_f, R_r))
            break;
        Lrear = j;
        i = (--i)%Q_LEN;
    }
    pthread_mutex_unlock(&LDQ_lock);
    pthread_mutex_lock(&SDQ_lock);
    for (int i = Srear; ;)
    {
        int j = (i-1)%Q_LEN;
        if (j == Srear || !is_prev_index(ROB_i, Stor_Q[j].rob_i, R_f, R_r))
            break;
        Srear = j;
        i = (--i)%Q_LEN;
    }
    pthread_mutex_unlock(&SDQ_lock);
    msg_log("After squash, Store front = " + to_string(Sfront) + " rear = " + to_string(Srear), 3);
}

void memory::SD_Q_automat()
{
    SDQ_next_vdd = 0;
    while (true)
    {
        at_rising_edge(SDQ_next_vdd);
        pthread_mutex_lock(&SDQ_lock);
        int CDB_i = fCDB.get_source();
        for (int i = Sfront; i != Srear;)
        {
            if (Stor_Q[i].SD_source == CDB_i)
            {
                msg_log("SD queue entry " + to_string(i) + " got the value, source ROB = " + to_string(CDB_i), 3);
                fCDB.get_val(&Stor_Q[i].val);
                Stor_Q[i].SD_source = -1;
            }
            i = (++i)%Q_LEN;
        }
        pthread_mutex_unlock(&SDQ_lock);
        at_falling_edge(SDQ_next_vdd);
        for (int i = Sfront; i != Srear;)
        {
            if (Stor_Q[i].addr != -1)
            {
                Stor_Q[i].ready = true;
                msg_log("Stor_Q entry i = " + to_string(i) + " ready, ROB = " + to_string(Stor_Q[Sfront].rob_i), 3);
            }
            i = (++i)%Q_LEN;
        }
    }
}

void memory::mem_automat()
{
    mem_next_vdd = 0;
    while (true)
    {
        at_rising_edge(mem_next_vdd);
        if (Sfront == Srear && Lfront == Lrear && sys_clk.is_instr_ended())
            sys_clk.end_mem();
        if (Sfront != Srear && Stor_Q[Sfront].ready && CPU_ROB->get_front() == Stor_Q[Sfront].rob_i)
            store(Stor_Q[Sfront]);
        else if(Lfront != Lrear && Load_Q[Lfront].ready)
            load(Load_Q[Lfront]);
        else
            at_falling_edge(mem_next_vdd);
    }
}

void init_main_mem()
{
    while(pthread_create(&main_mem.mem_handle, NULL, [](void *arg)->void*{main_mem.mem_automat(); return NULL;}, NULL));
    while(pthread_create(&main_mem.SDQ_handle, NULL, [](void *arg)->void*{main_mem.SD_Q_automat(); return NULL;}, NULL));
    clk_wait_list.push_back(&main_mem.mem_next_vdd);
    clk_wait_list.push_back(&main_mem.SDQ_next_vdd);
}