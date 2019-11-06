#include "mips.h"

config *CPU_cfg = nullptr;
instr_queue *instr_Q = nullptr;
ROB *CPU_ROB = nullptr;
pthread_t iss_unit;
vector<intAdder*> iAdder;
vector<flpAdder*> fAdder;
vector<flpMtplr*> fMtplr;


FU_CDB fCDB = FU_CDB();
unordered_map <string, int> RAT;
registor reg = registor(INT_REG_NUM, FP_REG_NUM);
clk_tick sys_clk = clk_tick();
vector<int*> clk_wait_list = {};
memory main_mem = memory(MEM_LEN);


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

void clk_tick::reset_prog_cyc()
{
    prog_cyc = 0;
}

int clk_tick::get_prog_cyc()
{
    return prog_cyc;
}

int all_unit_ready_for_one()
{
    int i = 1;
    for (const int* v : clk_wait_list)
    {
        if (!v)
        {
            err_log("null Ptr encountered in clk_wait_list");
            return -1;
        }
        i*=*v;
    }
    return i == 1;
}

int all_unit_ready_for_zero()
{
    int i = 0;
    for (const int* v : clk_wait_list)
    {
        if (!v)
        {
            err_log("null Ptr encountered in clk_wait_list");
            return -1;
        }
        i+=*v;
    }
    return i == 0;
}

bool clk_tick::oscillator(int freq)
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
            Sleep(500/freq);
        }
        prog_cyc++;
        vdd = 1;
        if (prog_cyc == 1)
            msg_log("Program Started\n", 0);
        pthread_cond_broadcast(&vdd_1);

        while (!all_unit_ready_for_zero())
        {
            string outp = "";
            for (auto a : clk_wait_list)
                outp += to_string(*a) + ",";
            msg_log("In waiting for vdd = 0, " + outp, 4);
            Sleep(500/freq);
        }
        vdd = 0;
        pthread_cond_broadcast(&vdd_0);
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

void at_rising_edge(mutex_t *lock, int &next_vdd)
{
    next_vdd = 1;
    pthread_mutex_lock(lock);
    while(sys_clk.get_vdd() == 0)
        pthread_cond_wait(&(sys_clk.vdd_1), lock);
    pthread_mutex_unlock(lock);
}

void at_falling_edge(mutex_t *lock, int &next_vdd)
{
    next_vdd = 0;
    pthread_mutex_lock(lock);
    while(sys_clk.get_vdd() == 1)
        pthread_cond_wait(&(sys_clk.vdd_0), lock);
    pthread_mutex_unlock(lock);
}

void start_sys_clk() // Starts the primal VDD clock and wait until it goes stable (covered 500 cycles)
{
    sys_clk.reset_prog_cyc();
    pthread_create(&(sys_clk.handle), NULL, [](void *arg)->void*{sys_clk.oscillator(500);return NULL;},NULL);
}

int main()
{
    if (!read_config_instrs("./inputTest.txt"))
        return -1;
    // init_main_mem();
    init_CPU_ROB();
    init_FUs();
    init_FU_CDB();
    init_issue_unit();
    msg_log("Threads count is: "+to_string(clk_wait_list.size()), 0);
    msg_log("instr buffer size is: " + to_string(instr_Q->size), 3);
    start_sys_clk();
    // int i = 110, li;
    // float f = 10.1, lf;
    // main_mem.enQ(SD, INTGR, 1, &i);
    // main_mem.enQ(SD, FLTP, 2, &f);
    // main_mem.enQ(LD, INTGR, 1, &li);
    // main_mem.enQ(LD, FLTP, 2, &lf);
    cin.get();
}