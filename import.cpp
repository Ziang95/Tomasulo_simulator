#include "import.h"

extern config *CPU_cfg;
extern clk_tick sys_clk;
extern instr *instr_Q;
extern registor reg;
extern memory main_mem;

void read_config_instrs(string path)
{
    if (sys_clk.get_prog_cyc())
    {
        err_log("Unsafe config modification, CPU is running");
        return;
    }
    if (CPU_cfg)
    {
        delete CPU_cfg;
        CPU_cfg = nullptr;
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

    string tmp;
    
    reg.clear();
    getline(file, line);
    istringstream iss(line);
    while (getline(iss, tmp))
    {
        size_t found = tmp.find('=');
        if (tmp[0] == 'R')
        {
            int val = stoi(tmp.substr(found+1, tmp.size() - found - 1));
            reg.set(tmp.substr(0, found), &val);
        }
        else
        {
            float val = stof(tmp.substr(found + 1, tmp.size() - found - 1));
            reg.set(tmp.substr(0, found), &val);
        }
    }
    #pragma region Set_mem_value
    getline(file, line);
    istringstream iss1(line);
    while (getline(iss1, tmp))
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

    if (instr_Q)
    {
        delete instr_Q;
        instr_Q = nullptr;
    }
    
    return;
}