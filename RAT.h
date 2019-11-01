#ifndef RAT_H
#define RAT_H

#include "mips.h"
using namespace std;

class registor
{
    private:
        int *intReg;
        float *flpReg;
    public:
        registor(int iRegSize, int fRegSize);
        ~registor();
        bool get(string name, void *ret);
        bool set(string name, void *val);
        void clear();
};

class RAT
{
    // Global definition of RAT for integer and floating Register Files
public:
    string R; //should be ROBx or Rx for integer.
    string F; //should be ROBx or Fx for float.
};

#endif