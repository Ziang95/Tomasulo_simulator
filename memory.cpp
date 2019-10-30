#include "memory.h"

using namespace std;

extern clk_tick sys_clk;
extern memory main_mem;

memory::memory(int sz)
{
    buf = new memCell[sz];
    bufStat = IDLE;
    size = sz;
    front = rear = 0;
    for (auto c: LSQ)
    {
        c.token = PTHREAD_COND_INITIALIZER;
    }
}

memory::~memory()
{
    delete []buf;
}

int memory::get_buf_stat(int addr)
{
    return bufStat;
}

bool memory::store(QEntry& entry) //This function is designed with "being called at rising edge" in mind
{
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    for (int i = 0; ; i++)
    {
        msg_log("Storing value to Addr="+to_string(entry.addr), 3);
        at_falling_edge(&clk);
        if (i == LD_STR_MEM_TIME - 1)
            break;
        at_rising_edge(&clk);
    }
    if (entry.fp)
        buf[entry.addr].f = *((float*)(entry.val));
    else
        buf[entry.addr].i = *((int*)(entry.val));
    CDB.fp = entry.fp;
    CDB.addr = entry.addr;
    CDB.val = buf[entry.addr];
    entry.done = true;
    msg_log("Value stored, Val="+to_string(entry.fp?(*(float*)entry.val):(*(int*)entry.val)), 3);
    pthread_cond_broadcast(&(entry.token));
    return true;
}

bool memory::load(QEntry& entry) //This function is designed with "being called at rising edge" in mind
{
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    if (CDB.addr == entry.addr)
    {
        msg_log("Forward store found, forwarding, Addr="+to_string(CDB.addr), 3);
        at_falling_edge(&clk);
        if (entry.fp)
            *((float*)(entry.val)) = CDB.val.f;
        else
            *((int*)(entry.val)) = CDB.val.i;
    }
    else
    {
        for (int i = 0; ; i++)
        {
            msg_log("Loading value from Addr="+to_string(entry.addr), 3);
            at_falling_edge(&clk);
            if (i == LD_STR_MEM_TIME - 1)
                break;
            at_rising_edge(&clk);
        }
        if (entry.fp)
            *((float*)(entry.val)) = buf[entry.addr].f;
        else
            *((int*)(entry.val)) = buf[entry.addr].i;
    }
    entry.done = true;
    msg_log("Value loaded, Val="+to_string(entry.fp?(*(float*)entry.val):(*(int*)entry.val)), 2);
    pthread_cond_broadcast(&entry.token);
    return true;
}

 QEntry* memory::enQ(bool store, bool fp, int addr, void* val)
{
    if (addr >= size)
        throw -1;
    int index = rear;
    rear = (++rear)%512;
    LSQ[index].addr = addr;
    LSQ[index].store = store;
    LSQ[index].fp = fp;
    LSQ[index].val = val;
    LSQ[index].done = false;
    return LSQ+index;
}

void memory::mem_automat()
{
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&clk);
        if (front != rear)
        {
            if (LSQ[front].store)
                store(LSQ[front]);
            else
                load(LSQ[front]);
            front = (++front)%512;
        }
        else
            at_falling_edge(&clk);
    }
}

bool init_main_mem()
{
    pthread_create(&main_mem.handle, NULL, [](void *arg)->void*{main_mem.mem_automat();}, NULL);
}