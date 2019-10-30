#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef void * (*threadFunc)(void *);

#include "mips.h"

typedef enum states {LOAD, STORE, IDLE} states;
typedef union memCell{
    int i;
    float f;
}memCell;
typedef struct QEntry{
    bool done;
    bool store;
    bool fp;
    void* val;
    int addr;
    cond_t token;
}QEntry;

class memory
{
    private:
        memCell *buf; //Stores values
        int size = 0; //Size of [buf]
        states bufStat;
        struct dataBus{
            bool fp;
            memCell val;
            int addr;
        }CDB;
        struct QEntry LSQ[512];
        int front, rear;

        bool store(struct QEntry& entry);
        bool load(struct QEntry& entry);
    public:
        pthread_t handle;
        memory(int sz); //Constructor
        ~memory();      //Destructor 
        int get_buf_stat(int addr);
        QEntry* enQ(bool store, bool fp, int addr, void* val); //Should be called at falling edge
        void mem_automat();
};

bool init_main_mem();

#endif
