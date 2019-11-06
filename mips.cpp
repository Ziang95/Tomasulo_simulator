#include "mips.h"

config *CPU_cfg = nullptr;
instr_queue *instr_Q = nullptr;
ROB *CPU_ROB = nullptr;
pthread_t iss_U;
vector<intAdder*> iAdder;
vector<flpAdder*> fAdder;
vector<flpMtplr*> fMtplr;


FU_CDB fCDB = FU_CDB();
unordered_map <string, int> RAT;
registor reg = registor(INT_REG_NUM, FP_REG_NUM);
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
        i*=*v; //This is a cute face LMAO
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
        i+=*v; //This is a cute face LMAO
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

void init_CPU_ROB()
{
    if (CPU_ROB)
        delete CPU_ROB;
    CPU_ROB = new ROB(*(CPU_cfg->ROB_num));
    clk_wait_list.push_back(&CPU_ROB->next_vdd);
    pthread_create(&CPU_ROB->handle, NULL, &ROB_thread_container, CPU_ROB);
}

void init_FUs()
{
    if (iAdder.size())
        for (auto &p : iAdder)
            delete p;
    if (fAdder.size())
        for (auto &p : fAdder)
            delete p;
    if (fMtplr.size())
        for (auto &p : fMtplr)
            delete p;
    iAdder = {};
    fAdder = {};
    fMtplr = {};
    for (int i = 0; i<CPU_cfg->int_add->fu_num; i++)
    {
        auto tmp = new intAdder();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &intAdder_thread_container, tmp);
        iAdder.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->fp_add->fu_num; i++)
    {
        auto tmp = new flpAdder();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &flpAdder_thread_container, tmp);
        fAdder.push_back(tmp);
    }
    for (int i = 0; i<CPU_cfg->fp_mul->fu_num; i++)
    {
        auto tmp = new flpMtplr();
        clk_wait_list.push_back(&tmp->next_vdd);
        pthread_create(&tmp->handle, NULL, &flpMlptr_thread_container, tmp);
        fMtplr.push_back(tmp);
    }
}

int main()
{
    read_config_instrs("./inputTest.txt");
    // init_main_mem();
    init_CPU_ROB();
    init_FU_CDB();
    init_FUs();
    init_issue_unit();
    msg_log("Threads number is: "+to_string(clk_wait_list.size()), 0);
    msg_log("instr buffer is: " + to_string(instr_Q->size), 3);
    Sleep(1000);
    start_sys_clk();
    // for (int i = 0; i<instr_Q->size; i++)
    // {
    //     const instr *tmp = instr_Q->getInstr();
    //     cout<<tmp->name<<' '<<tmp->dest<<' '<<tmp->oprnd1<<' '<<tmp->oprnd2<<' '<<tmp->imdt<<' '<<tmp->offset<<endl;
    //     instr_Q->ptr_advance();
    // }
    // int i = 110, li;
    // float f = 10.1, lf;
    // main_mem.enQ(SD, INTGR, 1, &i);
    // main_mem.enQ(SD, FLTP, 2, &f);
    // main_mem.enQ(LD, INTGR, 1, &li);
    // main_mem.enQ(LD, FLTP, 2, &lf);
    cin.get();
    Sleep(3567587328);
}