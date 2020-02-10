#include "./headers/mips.h"

config *CPU_cfg = nullptr;
instr_queue *instr_Q = nullptr;
ROB *CPU_ROB = nullptr;
BTB CPU_BTB = BTB();
branchCtrl brcUnit;

pthread_t iss_unit;
nopBublr* nopBub = nullptr;
vector<intAdder*> iAdder;
vector<flpAdder*> fAdder;
vector<flpMtplr*> fMtplr;
vector<ldsdUnit*> lsUnit;

FU_CDB fCDB = FU_CDB();
unordered_map <string, int> RAT;
registor *reg = new registor(REG_NUM);
clk_tick sys_clk = clk_tick();
vector<int*> clk_wait_list = {};
memory main_mem = memory(MEM_LEN);

vector<output_QEntry> CPU_output_Q = {};

int debug_level = 0;

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 3)
    {
        cout<<"Require 1 or 2 arguments"<<endl;
        return -1;
    }
    if (argc == 3)
        debug_level = stoi(argv[2]);
    if (!read_config_instrs(string(argv[1])))
        return -1;
    init_CPU_ROB();
    init_FUs();
    init_FU_CDB();
    init_main_mem();
    init_issue_unit();
    init_brcUnit();
    init_output_unit();
    msg_log("Threads count is: "+to_string(clk_wait_list.size()), 0);
    msg_log("instr buffer size is: " + to_string(instr_Q->size), 3);
    start_sys_clk();
    cin.get();
}