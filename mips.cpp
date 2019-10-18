#include "mips.h"
#include "memory.h"

using namespace std;

int pro_cyc; // The current cycle of program
clk_tick sys_clk = clk_tick(); //System clock
mutex_t cout_lock = PTHREAD_MUTEX_INITIALIZER;
mutex_t cerr_lock = PTHREAD_MUTEX_INITIALIZER;

clk_tick::clk_tick(){
    vdd_0 = vdd_1 = PTHREAD_COND_INITIALIZER;
    vdd = 0;
}

void clk_tick::set_vdd(bool val){
    vdd = val;
}

bool clk_tick::get_vdd(){
    return vdd;
}
bool clk_tick::oscillator(int freq){
    if (freq > 500){
        err_log("Frequency should be under 500");
        return false;
    }
    while (true)
    {
        set_vdd(1);
        pthread_cond_broadcast(&vdd_1);
        Sleep(500/freq);
        set_vdd(0);
        pthread_cond_broadcast(&vdd_0);
        Sleep(500/freq);
    }
}

void msg_log(string msg, int lvl){
    pthread_mutex_lock(&cout_lock);
    if (DEBUG_LEVEL>=lvl)
        cout<<'['<<__FILE__<<"-"<<__LINE__<<": pro_cyc="<<pro_cyc<<", vdd="<<sys_clk.get_vdd()<<']'<<msg<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void err_log(string err){
    pthread_mutex_lock(&cout_lock);
    cout<<'['<<__FILE__<<"-"<<__LINE__<<": pro_cyc="<<pro_cyc<<", vdd="<<sys_clk.get_vdd()<<']'<<err<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void at_rising_edge(clk_tick &clk, mutex_t *lock){
    pthread_mutex_lock(lock);
    while(clk.get_vdd() == 0)
        pthread_cond_wait(&(clk.vdd_1), lock);
    pthread_mutex_unlock(lock);
}

void at_falling_edge(clk_tick &clk, mutex_t *lock){
    pthread_mutex_lock(lock);
    while(clk.get_vdd() == 1)
        pthread_cond_wait(&(clk.vdd_0), lock);
    pthread_mutex_unlock(lock);
}

void init_sys_clk(){// Starts the primal VDD clock and wait until it goes stable (covered 500 cycles)
    pthread_t clock;
    pthread_create(&clock, NULL, [](void *arg)->void*{sys_clk.oscillator(250);return NULL;},NULL);
    msg_log("Waiting for sys_clk to be stable...", 0);
    int count = 0;
    {
        mutex_t tmp = PTHREAD_MUTEX_INITIALIZER;
        while(count<500){
            at_rising_edge(sys_clk, &tmp);
            count++;
            at_falling_edge(sys_clk, &tmp);
        }
        msg_log("sys_clk stablized", 0);
    }
}

int main(){
    init_sys_clk();
    cin.get();
}