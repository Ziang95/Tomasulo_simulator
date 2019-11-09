#include "memory.h"

extern clk_tick sys_clk;
extern memory main_mem;
extern config *CPU_cfg;
extern FU_CDB fCDB;
extern vector<int*> clk_wait_list;
extern ROB *CPU_ROB;

memory::memory(int sz)
{
    buf = new memCell[sz];
    memset(buf, 0, sz);
    size = sz;
    front = rear = 0;
    mem_CDB.source = -1;
    Q_lock = PTHREAD_MUTEX_INITIALIZER;
}

memory::~memory()
{
    delete []buf;
}

bool memory::store(LSQEntry& entry) //This function is designed with "being called at rising edge" in mind
{
    ROBEntry *R = CPU_ROB->get_entry(entry.rob_i);
    while (true)
    {
        if (CPU_ROB->get_front() >= entry.rob_i)
            break;
        at_falling_edge(next_vdd);
        if (!R->finished)
            return true;
        at_rising_edge(next_vdd);
    }
    R->output.commit = R->output.mem = sys_clk.get_prog_cyc();
    // instr_timeline_output(R);
    // CPU_ROB->ptr_advance();
    for (int i = 0; ; i++)
    {
        msg_log("Storing value to Addr="+to_string(entry.addr), 3);
        if (i == CPU_cfg->ld_str->mem_time - 1)
            break;
        at_falling_edge(next_vdd);
        at_rising_edge(next_vdd);
    }
    if (entry.type == FLTP)
        buf[entry.addr].f = entry.val->f;
    else
        buf[entry.addr].i = entry.val->f;
    mem_CDB.type = entry.type;
    mem_CDB.source = entry.addr;
    mem_CDB.value = buf[entry.addr];
    entry.done = true;
    at_falling_edge(next_vdd);
    msg_log("Value stored, Val="+to_string(entry.type == FLTP?entry.val->f : entry.val->i), 2);
    return true;
}

bool memory::load(LSQEntry& entry) //This function is designed with "being called at rising edge" in mind
{
    ROBEntry *R = CPU_ROB->get_entry(entry.rob_i);
    R->output.mem = sys_clk.get_prog_cyc();
    if (mem_CDB.source == entry.addr)
    {
        msg_log("Forward store found, forwarding, Addr="+to_string(mem_CDB.source), 3);
        at_falling_edge(next_vdd);
        if (entry.type == FLTP)
            entry.val->f = mem_CDB.value.f;
        else
            entry.val->i = mem_CDB.value.i;
    }
    else
    {
        for (int i = 0; ; i++)
        {
            msg_log("Loading value from Addr="+to_string(entry.addr), 3);
            if (i == CPU_cfg->ld_str->mem_time - 1)
                break;
            at_falling_edge(next_vdd);
            at_rising_edge(next_vdd);
        }
        if (entry.type == FLTP)
            *(float*)entry.val = buf[entry.addr].f;
        else
            *(int*)entry.val = buf[entry.addr].i;
    }
    fCDB.enQ(entry.type, entry.val, entry.rob_i);
    entry.done = true;
    at_falling_edge(next_vdd);
    msg_log("Value loaded, Val="+to_string(entry.type == FLTP?entry.val->f : entry.val->i), 2);
    return true;
}

const LSQEntry* memory::enQ(int rbi, opCode code, valType type, int addr, memCell *val)
{
    if (code != SD && code != LD)
        throw -1;
    if (addr >= size || addr<0)
        throw -2;
    pthread_mutex_lock(&Q_lock);
    if ((rear+1)%Q_LEN == front)
    {
        pthread_mutex_unlock(&Q_lock);
        throw -3;
    }
    int index = rear;
    rear = (++rear)%Q_LEN;
    LSQ[index].rob_i = rbi;
    LSQ[index].addr = addr;
    LSQ[index].code = code;
    LSQ[index].type = type;
    LSQ[index].val = val;
    LSQ[index].done = false;
    pthread_mutex_unlock(&Q_lock);
    return LSQ+index;
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

void memory::mem_automat()
{
    next_vdd = 0;
    while (true)
    {
        at_rising_edge(next_vdd);
        if (front != rear)
        {
            if (LSQ[front].code == SD)
                store(LSQ[front]);
            else
                load(LSQ[front]);
            front = (++front)%Q_LEN;
        }
        else
            at_falling_edge(next_vdd);
    }
}

void init_main_mem()
{
    int ret = 1;
    while(ret) ret = pthread_create(&main_mem.handle, NULL, [](void *arg)->void*{main_mem.mem_automat(); return NULL;}, NULL);
    clk_wait_list.push_back(&main_mem.next_vdd);
}