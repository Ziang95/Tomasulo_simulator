#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "mips.h"

class memory_32{
    private:
        int32_t *buffer;
        bool *busy;
        int size = 0;
    public:
        memory_32(int32_t *buf, bool *bus);
        void store(int32_t value, int addr);
};

#endif
