#include "mips.h"

config *CPU_cfg = nullptr;
instr *instr_Q = nullptr;

registor reg(INT_REG_NUM, FP_REG_NUM);

clk_tick sys_clk = clk_tick(); //System clock
memory main_mem = memory(MEM_LEN); //Main memory

static mutex_t cout_lock = PTHREAD_MUTEX_INITIALIZER;
static mutex_t cerr_lock = PTHREAD_MUTEX_INITIALIZER;


clk_tick::clk_tick()
{
    vdd_0 = vdd_1 = PTHREAD_COND_INITIALIZER;
    vdd = 0;
    prog_started = false;
    prog_cyc = 0;
}

bool clk_tick::get_vdd()
{
    return vdd;
}

int clk_tick::get_prog_cyc()
{
    return prog_cyc;
}

void clk_tick::start_prog()
{
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    at_falling_edge(&clk);
    pthread_cond_broadcast(&start);
    sys_clk.prog_cyc = 0;
    prog_started = true;
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
        if (prog_started)
            prog_cyc++;
        vdd = 1;
        if (prog_cyc == 1)
            msg_log("Program Started", 0);
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
        cout<<"[Pro_cyc="<<sys_clk.get_prog_cyc()<<", vdd="<<sys_clk.get_vdd()<<"] "<<msg<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void err_log(string err)
{
    pthread_mutex_lock(&cout_lock);
    cout<<"[Pro_cyc="<<sys_clk.get_prog_cyc()<<", vdd="<<sys_clk.get_vdd()<<"] "<<err<<endl;
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
    pthread_create(&(sys_clk.handle), NULL, [](void *arg)->void*{sys_clk.oscillator(100);return NULL;},NULL);
    msg_log("Waiting for sys_clk to be stable...", 0);
    int count = 0;
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    while(count<100)
    {
        at_rising_edge(&clk);
        count++;
        at_falling_edge(&clk);
    }
    msg_log("sys_clk stablized", 0);
}

int main()
{
    init_sys_clk();
    init_main_mem();
    sys_clk.start_prog();
    read_config_instrs("./inputTest.txt");
    int i = 110, li;
    float f = 10.1, lf;
    main_mem.enQ(STORE, INTGR, 1, &i);
    main_mem.enQ(STORE, FLTP, 2, &f);
    main_mem.enQ(LOAD, INTGR, 1, &li);
    main_mem.enQ(LOAD, FLTP, 2, &lf);
    cin.get();
}