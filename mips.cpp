#include "mips.h"

config *CPU_cfg = nullptr;
instr_queue *instr_Q = nullptr;
ROB *CPU_ROB = nullptr;

FU_CDB fCDB = FU_CDB();
unordered_map <string, int> RAT;
registor reg(INT_REG_NUM, FP_REG_NUM);
clk_tick sys_clk = clk_tick(); //System clock
vector<int*> clk_wait_list = {};
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

void clk_tick::reset_prog_cyc()
{
    prog_cyc = 0;
}

int clk_tick::get_prog_cyc()
{
    return prog_cyc;
}

int all_unit_ready_for()
{
    int e = 1;
    for (auto a : clk_wait_list)
        (e*=*a); //This is a cute face LMAO
    return e;
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
        prog_cyc++;
        vdd = 1;
        if (prog_cyc == 1)
            msg_log("Program Started", 0);
        pthread_cond_broadcast(&vdd_1);
        while (all_unit_ready_for() != 0)
            Sleep(500/freq);
        vdd = 0;
        pthread_cond_broadcast(&vdd_0);
        while (all_unit_ready_for() != 1)
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
    read_config_instrs("./inputTest.txt");
    init_main_mem();
    start_sys_clk();
    // for (int i = 0; i<instr_Q->size; i++)
    // {
    //     const instr *tmp = instr_Q->getInstr();
    //     cout<<tmp->code<<' '<<tmp->dest<<' '<<tmp->oprnd1<<' '<<tmp->oprnd2<<' '<<tmp->imdt<<' '<<tmp->offset<<endl;
    //     instr_Q->ptr_advance();
    // }
    int i = 110, li;
    float f = 10.1, lf;
    main_mem.enQ(SD, INTGR, 1, &i);
    main_mem.enQ(SD, FLTP, 2, &f);
    main_mem.enQ(LD, INTGR, 1, &li);
    main_mem.enQ(LD, FLTP, 2, &lf);
    cin.get();
}