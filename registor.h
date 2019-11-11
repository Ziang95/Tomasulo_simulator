#ifndef REGISTOR_H
#define REGISTOR_H

using namespace std;

#include "mips.h"

class registor
{
    private:
        int *intReg;
        float *flpReg;
    public:
        const int RegSize;
        registor(int _RegSize);
        ~registor();
        bool get(string name, memCell &ret);
        bool set(string name, memCell val);
        void clear();
};

#endif