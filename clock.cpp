#include ".\headers\clock.h"

extern vector<int*> clk_wait_list;
extern int debug_level;
extern clk_tick sys_clk;

mutex_t cout_lock = PTHREAD_MUTEX_INITIALIZER;
static mutex_t boradcast_lock = PTHREAD_MUTEX_INITIALIZER;

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

void clk_tick::reset_prog_cyc()
{
    prog_cyc = 0;
}

int clk_tick::get_prog_cyc()
{
    return prog_cyc;
}

bool clk_tick::is_instr_ended()
{
    return instr_ended;
}

void clk_tick::end_instr()
{
    instr_ended = true;
}

bool clk_tick::is_mem_ended()
{
    return mem_ended;
}

void clk_tick::end_mem()
{
    mem_ended = true;
}

int all_unit_ready_for_one()
{
    int r = 1;
    for (const int* v : clk_wait_list)
    {
        if (!v)
        {
            err_log("null Ptr encountered in clk_wait_list");
            return -1;
        }
        r *= *v;
    }
    return r == 1;
}

int all_unit_ready_for_zero()
{
    int r = 0;
    for (const int* v : clk_wait_list)
    {
        if (!v)
        {
            err_log("null Ptr encountered in clk_wait_list");
            return -1;
        }
        r += *v;
    }
    return r == 0;
}

bool clk_tick::clk_automat(int freq)
{
    if (freq > 500)
    {
        err_log("Frequency should be no greater than 500");
        return false;
    }
    while (true)
    {
        while (!all_unit_ready_for_one())
        {
            string outp = "";
            for (auto a : clk_wait_list)
                outp += to_string(*a) + ",";
            msg_log("In waiting for vdd = 1, " + outp, 4);
            #ifdef _WIN32
            Sleep(500/freq);
            #else
            usleep(500/freq);
            #endif
        }
        pthread_mutex_lock(&boradcast_lock);
        vdd = 1;
        prog_cyc++;
        if (prog_cyc == 1)
            msg_log("Program Started\n", 0);
        pthread_cond_broadcast(&vdd_1);
        pthread_mutex_unlock(&boradcast_lock);
        while (!all_unit_ready_for_zero())
        {
            string outp = "";
            for (auto a : clk_wait_list)
                outp += to_string(*a) + ",";
            msg_log("In waiting for vdd = 0, " + outp, 4);
            #ifdef _WIN32
            Sleep(500/freq);
            #else
            usleep(500/freq);
            #endif
        }
        pthread_mutex_lock(&boradcast_lock);
        vdd = 0;
        pthread_cond_broadcast(&vdd_0);
        pthread_mutex_unlock(&boradcast_lock);
        if (instr_ended && mem_ended)
            return true;
    }
}

void msg_log(string msg, int lvl)
{
    if (debug_level < lvl) return;
    pthread_mutex_lock(&cout_lock);
    cout<<"[Pro_cyc="<<sys_clk.get_prog_cyc()<<", vdd="<<sys_clk.get_vdd()<<"] "<<msg<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void err_log(string err)
{
    pthread_mutex_lock(&cout_lock);
    cout<<"[Pro_cyc="<<sys_clk.get_prog_cyc()<<", vdd="<<sys_clk.get_vdd()<<"] "<<err<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void at_rising_edge(int &next_vdd)
{
    pthread_mutex_lock(&boradcast_lock);
    next_vdd = 1;
    while(sys_clk.get_vdd() == 0)
        pthread_cond_wait(&(sys_clk.vdd_1), &boradcast_lock);
    pthread_mutex_unlock(&boradcast_lock);
}

void at_falling_edge(int &next_vdd)
{
    pthread_mutex_lock(&boradcast_lock);
    next_vdd = 0;
    while(sys_clk.get_vdd() == 1)
        pthread_cond_wait(&(sys_clk.vdd_0), &boradcast_lock);
    pthread_mutex_unlock(&boradcast_lock);
}

void start_sys_clk() // Starts the primal VDD clock and wait until it goes stable (covered 500 cycles)
{
    sys_clk.reset_prog_cyc();
    while(pthread_create(&(sys_clk.handle), NULL, [](void *arg)->void*{sys_clk.clk_automat(500);return NULL;},NULL));
}