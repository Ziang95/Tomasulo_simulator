#include "registor.h"

extern clk_tick sys_clk;

registor::registor(int iRegSize, int fRegSize)
{
    intReg = new int[iRegSize];
    intReg[0] = 0;
    flpReg = new float[fRegSize];
}

registor::~registor()
{
    if (intReg) delete[] intReg;
    if (flpReg) delete[] flpReg;
}

bool registor::get(string name, memCell &ret)
{
    int index = stoi(name.substr(1, name.size() - 1));
    if (index >= INT_REG_NUM)
    {
        err_log(name + ": Reg access out of range");
        return false;
    }
    if (name[0] == 'R')
        ret.i = intReg[index];
    else if (name[0] == 'F')
        ret.f = flpReg[index];
    else
    {
        err_log("Reg name error");
        return false;
    }
    return true;
}

bool registor::set(string name, memCell val)
{
    int index = stoi(name.substr(1, name.size() - 1));
    if (index >= INT_REG_NUM)
    {
        err_log(name + ": Reg access out of range");
        return false;
    }
    if (name[0] == 'R')
    {
        if (index == 0)
        {
            err_log("R0 can't be modified");
            return false;
        }
        intReg[index] = val.i;
    }
    else if (name[0] == 'F')
        flpReg[index] = val.f;
    else
    {
        err_log("Reg name error");
        return false;
    }
    return true;
}

void registor::clear()
{
    for (int i = 0; i<INT_REG_NUM; i++)
        intReg[i] = 0;
    for (int i = 0; i<FP_REG_NUM; i++)
        flpReg[i] = 0;
    return;
}