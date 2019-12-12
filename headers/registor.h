#ifndef REGISTOR_H
#define REGISTOR_H

using namespace std;

#include "mips.h"

class registor
{
    private:
        int *intReg;                            //The integer registor array
        float *flpReg;                          //The float point registor array
    public:
        const int RegSize;                      //The length of registor file
        registor(int _RegSize);                 //Constructor
        ~registor();                            //Destructor
        bool get(string name, memCell &ret);    //Get value and store it in ret
        bool set(string name, memCell val);     //Store value from val
        void clear();                           //Clear all values
};

#endif