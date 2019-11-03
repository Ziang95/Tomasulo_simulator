#include "memory.h"

extern clk_tick sys_clk;
extern memory main_mem;
extern config *CPU_cfg;

memory::memory(int sz)
{
    buf = new memCell[sz];
    size = sz;
    front = rear = 0;
    CDB.addr = -1;
    for (auto c: LSQ)
    {
        c.token = PTHREAD_COND_INITIALIZER;
    }
}

memory::~memory()
{
    delete []buf;
}

bool memory::store(QEntry& entry) //This function is designed with "being called at rising edge" in mind
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    for (int i = 0; ; i++)
    {
        msg_log("Storing value to Addr="+to_string(entry.addr), 3);
        at_falling_edge(&lock);
        if (i == CPU_cfg->ld_str->mem_time - 1)
            break;
        at_rising_edge(&lock);
    }
    if (entry.type == FLTP)
        buf[entry.addr].f = *((float*)entry.val);
    else
        buf[entry.addr].i = *((int*)entry.val);
    CDB.type = entry.type;
    CDB.addr = entry.addr;
    CDB.val = buf[entry.addr];
    entry.done = true;
    msg_log("Value stored, Val="+to_string(entry.type == FLTP?(*(float*)entry.val):(*(int*)entry.val)), 2);
    pthread_cond_broadcast(&(entry.token));
    return true;
}

bool memory::load(QEntry& entry) //This function is designed with "being called at rising edge" in mind
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    if (CDB.addr == entry.addr)
    {
        msg_log("Forward store found, forwarding, Addr="+to_string(CDB.addr), 3);
        at_falling_edge(&lock);
        if (entry.type == FLTP)
            *((float*)entry.val) = CDB.val.f;
        else
            *((int*)entry.val) = CDB.val.i;
    }
    else
    {
        for (int i = 0; ; i++)
        {
            msg_log("Loading value from Addr="+to_string(entry.addr), 3);
            at_falling_edge(&lock);
            if (i == CPU_cfg->ld_str->mem_time - 1)
                break;
            at_rising_edge(&lock);
        }
        if (entry.type == FLTP)
            *((float*)entry.val) = buf[entry.addr].f;
        else
            *((int*)entry.val) = buf[entry.addr].i;
    }
    entry.done = true;
    msg_log("Value loaded, Val="+to_string(entry.type == FLTP?(*(float*)entry.val):(*(int*)entry.val)), 2);
    pthread_cond_broadcast(&entry.token);
    return true;
}

 QEntry* memory::enQ(opCode code, valType type, int addr, void* val)
{
    if (code != SD && code != LD)
        throw -1;
    if (addr >= size || addr<0)
        throw -2;
    int index = rear;
    rear = (++rear)%512;
    LSQ[index].addr = addr;
    LSQ[index].code = code;
    LSQ[index].type = type;
    LSQ[index].val = val;
    LSQ[index].done = false;
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
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock);
        if (front != rear)
        {
            if (LSQ[front].code == SD)
                store(LSQ[front]);
            else
                load(LSQ[front]);
            front = (++front)%512;
        }
        else
            at_falling_edge(&lock);
    }
}

void init_main_mem()
{
    pthread_create(&main_mem.handle, NULL, [](void *arg)->void*{main_mem.mem_automat();return NULL;}, NULL);
}