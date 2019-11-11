#include "import.h"

extern config *CPU_cfg;
extern clk_tick sys_clk;
extern instr_queue *instr_Q;
extern registor *reg;
extern memory main_mem;

opCode get_opcode(string opName)
{
    for (char &c : opName)
        c = toupper(c);
    if (opName == "ADD")
        return ADD;
    else if (opName == "ADD.D")
        return ADD_D;
    else if (opName == "ADDI")
        return ADDI;
    else if (opName == "SUB")
        return SUB;
    else if (opName == "SUB.D")
        return SUB_D;
    else if (opName == "MULT.D")
        return MUL_D;
    else if (opName == "BEQ")
        return BEQ;
    else if (opName == "BNE")
        return BNE;
    else if (opName == "LD")
        return LD;
    else if (opName == "SD")
        return SD;
    else if (opName == "NOP")
        return NOP;
    else
        return ERR;
}

bool read_config_instrs(string path)
{
    if (sys_clk.get_prog_cyc())
    {
        err_log("Unsafe config modification, CPU is running");
        return false;
    }
    msg_log("Reading input file " + path, 1);
    ifstream file;
    string line, h, h1, h2; //h = head
    int p[4]; //p = params
    config *cfg_ret = new config();
    try{file.open(path);}
    catch(const ifstream::failure& e)
    {
        err_log("Exception occurred when opening file "+path);
        return false;
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
        return false;
    }
    CPU_cfg = cfg_ret;
    msg_log("CPU config read", 0);
#pragma endregion
#pragma region read_ini_reg_values
    string tmp;
    reg->clear();
    do {getline(file, line);}while(line.empty()||line.size()<4);
    line.erase(remove(line.begin(),line.end(),' '),line.end());
    line.erase(remove(line.begin(),line.end(),'\t'),line.end());
    istringstream iss(line);
    while (getline(iss, tmp, ','))
    {
        size_t e = tmp.find('=');
        size_t f = tmp.find('F');
        size_t i = tmp.find('R');
        memCell val;
        if (i != string::npos)
        {
            if (tmp.find('.') != string::npos)
            {
                err_log("Error: Trying to assign float value to an int reg");
                return -1;
            }
            val.i = stoi(tmp.substr(e + 1, tmp.size() - e - 1));
            reg->set(tmp.substr(i, e), val);
        }
        else if (f != string::npos)
        {
            val.f = stof(tmp.substr(e + 1, tmp.size() - e - 1));
            reg->set(tmp.substr(f, e), val);
        }
        else
        {
            err_log("IDK what's going on, better check your reg init line");
            return -1;
        }
        
    }
    msg_log("Ini reg values read", 2);
#pragma endregion
#pragma region read_ini_mem_values
    do{getline(file, line);}while(line.empty()||line.size()<4);
    line.erase(remove(line.begin(),line.end(),' '),line.end());
    line.erase(remove(line.begin(),line.end(),'\t'),line.end());
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
    msg_log("Ini mem values read", 2);
#pragma endregion
#pragma region read_instrs
    if (instr_Q)
        delete instr_Q;
    vector <instr> tmp_Q = {};
    string opName, dest, op1, op2, ofst, imm;
    while(getline(file, line))
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
            switch (code)
            {
            case ADD:
            case ADD_D:
            case SUB:
            case SUB_D:
            case MUL_D:
            {
                getline(s, dest, ',');
                getline(s, op1, ',');
                getline(s, op2, ',');
                dest.erase(remove(dest.begin(), dest.end(), ' '), dest.end());
                op1.erase(remove(op1.begin(), op1.end(), ' '), op1.end());
                op2.erase(remove(op2.begin(), op2.end(), ' '), op2.end());
                dest.erase(remove(dest.begin(), dest.end(), '\t'), dest.end());
                op1.erase(remove(op1.begin(), op1.end(), '\t'), op1.end());
                op2.erase(remove(op2.begin(), op2.end(), '\t'), op2.end());
                tmp_i.code = code;
                tmp_i.dest = dest;
                tmp_i.oprnd1 = op1;
                tmp_i.oprnd2 = op2;
                break;
            }
            case ADDI:
            {
                getline(s, dest, ',');
                getline(s, op1, ',');
                getline(s, imm, ',');
                dest.erase(remove(dest.begin(), dest.end(), ' '), dest.end());
                op1.erase(remove(op1.begin(), op1.end(), ' '), op1.end());
                imm.erase(remove(imm.begin(), imm.end(), ' '), imm.end());
                dest.erase(remove(dest.begin(), dest.end(), '\t'), dest.end());
                op1.erase(remove(op1.begin(), op1.end(), '\t'), op1.end());
                imm.erase(remove(imm.begin(), imm.end(), '\t'), imm.end());
                tmp_i.code = code;
                tmp_i.dest = dest;
                tmp_i.oprnd1 = op1;
                tmp_i.imdt = stoi(imm);
                break;
            }
            case BNE:
            case BEQ:
            {
                getline(s, op1, ',');
                getline(s, op2, ',');
                getline(s, ofst, ',');
                op1.erase(remove(op1.begin(), op1.end(), ' '), op1.end());
                op2.erase(remove(op2.begin(), op2.end(), ' '), op2.end());
                ofst.erase(remove(ofst.begin(), ofst.end(), ' '), ofst.end());
                op1.erase(remove(op1.begin(), op1.end(), '\t'), op1.end());
                op2.erase(remove(op2.begin(), op2.end(), '\t'), op2.end());
                ofst.erase(remove(ofst.begin(), ofst.end(), '\t'), ofst.end());
                tmp_i.code = code;
                tmp_i.oprnd1 = op1;
                tmp_i.oprnd2 = op2;
                tmp_i.offset = stoi(ofst);
                break;
            }
            case LD:
            case SD:
            {
                getline(s, dest, ',');
                getline(s, ofst, ',');
                dest.erase(remove(dest.begin(), dest.end(), ' '), dest.end());
                ofst.erase(remove(ofst.begin(), ofst.end(), ' '), ofst.end());
                dest.erase(remove(dest.begin(), dest.end(), '\t'), dest.end());
                ofst.erase(remove(ofst.begin(), ofst.end(), '\t'), ofst.end());
                tmp_i.code = code;
                tmp_i.dest = dest;
                size_t lb = ofst.find('(');
                size_t rb = ofst.find(')');
                tmp_i.oprnd1 = ofst.substr(lb + 1, rb - lb - 1);
                tmp_i.offset = stoi(ofst.substr(0, lb));
                break;
            }
            case NOP:
            {
                tmp_i.code = NOP;
                break;
            }
            case ERR:
            {
                err_log("Instruction opCode error, please check input file " + path);
                return false;
                break;
            }
            default:
                break;
            }
            tmp_Q.push_back(tmp_i);
        }
    }
    instr_Q = new instr_queue(tmp_Q);
#pragma endregion
    msg_log("Read complete!", 1);
    file.close();
    return true;
}