#include "mips.h"

using namespace std;

int pro_cyc; // The current cycle of program
clk_tick sys_clk = clk_tick(); //System clock
memory_8 main_mem = memory_8(MEM_LEN); //Main memory 
mutex_t cout_lock = PTHREAD_MUTEX_INITIALIZER;
mutex_t cerr_lock = PTHREAD_MUTEX_INITIALIZER;

clk_tick::clk_tick()
{
    vdd_0 = vdd_1 = PTHREAD_COND_INITIALIZER;
    vdd = 0;
}

bool clk_tick::get_vdd()
{
    return vdd;
}
bool clk_tick::oscillator(int freq)
{
    if (freq > 500)
    {
        err_log("Frequency should be under 500");
        return false;
    }
    while (true)
    {
        vdd = 1;
        pthread_cond_broadcast(&vdd_1);
        Sleep(500/freq);
        vdd = 0;
        pthread_cond_broadcast(&vdd_0);
        Sleep(500/freq);
    }
}

void msg_log(string msg, int lvl)
{
    pthread_mutex_lock(&cout_lock);
    if (DEBUG_LEVEL>=lvl)
        cout<<'['<<__FILE__<<"-"<<__LINE__<<": pro_cyc="<<pro_cyc<<", vdd="<<sys_clk.get_vdd()<<']'<<msg<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void err_log(string err)
{
    pthread_mutex_lock(&cout_lock);
    cout<<'['<<__FILE__<<"-"<<__LINE__<<": pro_cyc="<<pro_cyc<<", vdd="<<sys_clk.get_vdd()<<']'<<err<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void at_rising_edge(mutex_t *lock)
{
    pthread_mutex_lock(lock);
    while(sys_clk.get_vdd() == 0)
        pthread_cond_wait(&(sys_clk.vdd_1), lock);
    pthread_mutex_unlock(lock);
}

void at_falling_edge(mutex_t *lock)
{
    pthread_mutex_lock(lock);
    while(sys_clk.get_vdd() == 1)
        pthread_cond_wait(&(sys_clk.vdd_0), lock);
    pthread_mutex_unlock(lock);
}

void init_sys_clk() // Starts the primal VDD clock and wait until it goes stable (covered 500 cycles)
{
    pthread_t clock;
    pthread_create(&clock, NULL, [](void *arg)->void*{sys_clk.oscillator(100);return NULL;},NULL);
    msg_log("Waiting for sys_clk to be stable...", 0);
    int count = 0;
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    while(count<500){
        at_rising_edge(&clk);
        count++;
        at_falling_edge(&clk);
    }
    msg_log("sys_clk stablized", 0);
}

int main()
{
    init_sys_clk();
    cin.get();
}