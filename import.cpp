#include "import.h"

extern config *CPU_cfg;
extern clk_tick sys_clk;
extern instr *instr_Q;
extern registor reg;
extern memory main_mem;

opCode get_opcode(string opName)
{
    for (char &c : opName)
        c = toupper(c);
    if (opName == "ADD")
        return opCode::ADD;
    else if (opName == "ADD.D")
        return opCode::ADD_D;
    else if (opName == "ADDI")
        return opCode::ADDI;
    else if (opName == "SUB")
        return opCode::SUB;
    else if (opName == "SUB.D")
        return opCode::SUB_D;
    else if (opName == "MULT.D")
        return opCode::MUL_D;
    else if (opName == "BEQ")
        return opCode::BEQ;
    else if (opName == "BNE")
        return opCode::BNE;
    else if (opName == "LD")
        return opCode::LD;
    else if (opName == "SD")
        return opCode::SD;
    else
        return opCode::ERR;
}

void read_config_instrs(string path)
{
    if (sys_clk.get_prog_cyc())
    {
        err_log("Unsafe config modification, CPU is running");
        return;
    }
    ifstream file;
    string line, h, h1, h2; //h = head
    int p[4]; //p = params
    config *cfg_ret = new config();
    try{file.open(path);}
    catch(const ifstream::failure& e)
    {
        err_log("Exception occurred when opening file "+path);
        return;
    }
#pragma region read_CPU_config
    if (CPU_cfg)
        delete CPU_cfg;
    while(getline(file, line))
    {
        if (line.empty())
            continue;
        else
        {
            istringstream s(line);
            s>>h1>>h2;
            h = h1 + ' ' + h2;
            if (h == "# of")
                continue;
            if (h == "Integer adder")
            {
                s>>p[0]>>p[1]>>p[3];
                cfg_ret->int_add = new instr_param(p[0], p[1], 0, p[3]);
            }
            else if (h == "FP adder")
            {
                s>>p[0]>>p[1]>>p[3];
                cfg_ret->fp_add = new instr_param(p[0], p[1], 0, p[3]);
            }
            else if (h == "FP multiplier")
            {
                s>>p[0]>>p[1]>>p[3];
                cfg_ret->fp_mul = new instr_param(p[0], p[1], 0, p[3]);
            }
            else if (h == "Load/store unit")
            {
                s>>p[0]>>p[1]>>p[2]>>p[3];
                cfg_ret->ld_str = new instr_param(p[0], p[1], p[2], p[3]);
            }
            else if (h == "ROB entries")
            {
                s>>line>>p[0];
                cfg_ret->ROB_num = new int(p[0]);
            }
            else
                break;
        }
    }
    if (!cfg_ret->fp_add || !cfg_ret->fp_mul || !cfg_ret->int_add || !cfg_ret->ROB_num || !cfg_ret->ld_str)
    {
        err_log("Config file is incomplete");
        return;
    }
    CPU_cfg = cfg_ret;
#pragma endregion
#pragma region read_ini_reg_values
    string tmp;
    reg.clear();
    getline(file, line);
    istringstream iss(line);
    while (getline(iss, tmp, ','))
    {
        size_t e = tmp.find('=');
        size_t f = tmp.find('F');
        size_t i = tmp.find('R');
        if (f == string::npos)
        {
            int val = stoi(tmp.substr(e + 1, tmp.size() - e - 1));
            reg.set(tmp.substr(i, e), &val);
        }
        else
        {
            float val = stof(tmp.substr(e + 1, tmp.size() - e - 1));
            reg.set(tmp.substr(f, e), &val);
        }
    }
#pragma endregion
#pragma region read_ini_mem_values
    getline(file, line);
    istringstream iss1(line);
    while (getline(iss1, tmp, ','))
    {
        size_t e = tmp.find('=');
        size_t lb = tmp.find('[');
        size_t rb = tmp.find(']');
        size_t flt = tmp.find('.');
        if (flt == string::npos)
        {
            int val = stoi(tmp.substr(e + 1, tmp.size() - e - 1));
            main_mem.setMem(INTGR, stoi(tmp.substr(lb+1, rb-lb-1)), &val);
        }
        else
        {
            float val = stof(tmp.substr(e + 1, tmp.size() - e - 1));
            main_mem.setMem(FLTP, stoi(tmp.substr(lb+1, rb-lb-1)), &val);
        }
    }
#pragma endregion
#pragma region read_instrs
    if (instr_Q)
        delete instr_Q;
    vector <instr> tmp_Q = {};
    string opName;
    while(:getline(file, line))
    {
        if (line.empty())
            continue;
        else
        {
            instr tmp_i;
            tmp_i.name = line;
            istringstream s(line);
            s>>opName;
            opCode code = get_opcode(opName);
        }
    }
#pragma endregion
    return;
}