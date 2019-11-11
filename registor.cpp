#include "registor.h"

extern clk_tick sys_clk;

registor::registor(int _RegSize):RegSize(_RegSize)
{
    intReg = new int[_RegSize];
    memset(intReg, 0, _RegSize);
    intReg[0] = 0;
    flpReg = new float[_RegSize];
    for (int i = 0; i<RegSize; i++)
        flpReg[i] = 0.;
}

registor::~registor()
{
    if (intReg) delete[] intReg;
    if (flpReg) delete[] flpReg;
}

bool registor::get(string name, memCell &ret)
{
    int index = stoi(name.substr(1, name.size() - 1));
    if (index >= RegSize)
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
    if (index >= RegSize)
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
    for (int i = 0; i<RegSize; i++)
        intReg[i] = 0;
    for (int i = 0; i<RegSize; i++)
        flpReg[i] = 0;
    return;
}