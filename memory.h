#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "mips.h"

class memory_8 
/*This class define the behaviour of memory unit in MEM stage, 
(everything happens after the falling edge of the last EXE stage cycle)
*/
{
    private:
        int8_t *buf; //Stores values
        int *bufStat; //State of each buf entry, positive val stands for the rest LOAD cycle, negative val stands for rest STORE cycle
        mutex_t *bufStatMutex; //Prevent race condition between threads
        int size = 0; //Size of [buf]
    public:
        cond_t *bufBC; //Used by [buf] to broadcast its value when finishing STORE
        memory_8(int sz); //Constructor
        ~memory_8(); //Destructor
        int get_buf_stat(int addr);
        bool store(int8_t value, int addr); //Store value in memory, note that this function can only be called AT FALLING EDGE!
        bool load(int8_t &ret, int addr); //Load value from memory, note that this function can only be called AT FALLING EDGE!
        void load_cyc_decre();
};

#endif
