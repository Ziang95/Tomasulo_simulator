#include "memory.h"

using namespace std;

extern int pro_cyc;
extern clk_tick sys_clk;

memory_8::memory_8(int sz)
{
    buf = new int8_t[sz];
    bufStat = new int[sz];
    bufBC = new cond_t[sz];
    bufStatMutex = new mutex_t[sz];
    size = sz;

    memset(bufStat, 0, size*sizeof(int));
    for (int i = 0; i<size; i++)
        bufBC[i] = PTHREAD_COND_INITIALIZER;
    for (int i = 0; i<size; i++)
        bufStatMutex[i] = PTHREAD_MUTEX_INITIALIZER;
}

memory_8::~memory_8()
{
    delete []buf;
    delete []bufStat;
    delete []bufBC;
    delete []bufStatMutex;
}

int memory_8::get_buf_stat(int addr)
{
    return bufStat[addr];
}

bool memory_8::store(int8_t value, int addr) //This function is designed with "being called at falling edge" in mind
{
    if (addr >= size)
    {
        err_log("Store failed, Memory access out of range");
        return false;
    }
    if (buf[addr] < 0)
    {
        err_log("Store failed, value is being stored at ADDR="+to_string(addr));
        return false;
    }
    bufStat[addr] = -LD_STR_MEM_TIME;
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    for (int i = 0; i<LD_STR_MEM_TIME; i++)
    {
        at_rising_edge(&clk);
        bufStat[addr]++;
        at_falling_edge(&clk);
    }
    pthread_cond_broadcast(&bufBC[addr]);
    buf[addr] = value;
    return true;
}

bool memory_8::load(int8_t &ret, int addr) //This function is designed with "being called at falling edge" in mind
{
    if (addr >= size)
    {
        err_log("Load failed, Memory access out of range");
        return false;
    }
    if (buf[addr] < 0) //Forward value from preceding STORE
    {
        mutex_t waitStore = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&waitStore);
        while(buf[addr]<0)
            pthread_cond_wait(&bufBC[addr], &waitStore);
        pthread_mutex_unlock(&waitStore);
        ret = buf[addr];
        return true;
    }
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&bufStatMutex[addr]);
    bufStat[addr] = LD_STR_MEM_TIME; //No matter how many LOAD happens in parallel, the rest cycle will always be determined by the last LOAD
    pthread_mutex_unlock(&bufStatMutex[addr]);
    for (int i = 0; i<LD_STR_MEM_TIME; i++)
    {
        at_rising_edge(&clk);
        msg_log("Loading value from ADDr="+to_string(addr), 3);
        at_falling_edge(&clk);
    }
    ret = buf[addr];
    return true;
}

void memory_8::load_cyc_decre(){
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&clk);
        for (int i = 0; i<size; i++)
            if (bufStat[i] > 0)
                bufStat[i]--;
        at_falling_edge(&clk);
    }
}