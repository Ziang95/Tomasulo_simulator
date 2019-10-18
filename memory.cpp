#include "memory.h"

using namespace std;

extern int pro_cyc;
extern clk_tick sys_clk;

memory_32::memory_32(int32_t *buf, bool *bus){
    buffer = buf;
    busy = bus;
    size = sizeof(buf);
}

void memory_32::store(int32_t value, int addr){
    if (addr >= size){
        err_log("Memory access out of range");
    }
}