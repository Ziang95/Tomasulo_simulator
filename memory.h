#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef void * (*threadFunc)(void *);
typedef enum valType{INTGR,FLTP}valType;
typedef union memCell
{
    int i;
    float f;
}memCell;
typedef struct
{
    valType type;
    memCell value;
    int source;
}CDB;

#include "instruction.h"

typedef struct QEntry
{
    bool done;                          //Indicates whether the entry operation is finished
    opCode code;                        //Enum type, should be LOAD or STORE
    valType type;                       //Value type can either be float 
    void* val;                          //Pointer of the value to store FROM of to load TO, can be int* or float*
    int addr;                           //Address to store to or load FROM
    cond_t token;                       //When this queue entry is finished, it broadcasts its status to all threads waiting outside
}QEntry;

#include "mips.h"

class memory
{
    private:
        memCell *buf;                   //Memory storage buffer, can store either Integer or Float Point value
        int size = 0;                   //Size of [buf]
        CDB mem_CDB;
        struct QEntry LSQ[128];         //Load/Store queue
        int front, rear;                //Control pointers of circular queue [LSQ]
        bool store(QEntry& entry);      //Store the value to the address saved in LSQ entry
        bool load(QEntry& entry);       //Load the value from address saved in LSQ entry
    public:
        int next_vdd;
        pthread_t handle;               //The handle of thread running mem_automat()
        memory(int sz);                 //Constructor
        ~memory();                      //Destructor 
        bool setMem(valType type, int addr, void* val);
        const QEntry* enQ(opCode code, valType type, int addr, void* val); //Enqueue function, Should only be called at falling edges
        void mem_automat();             //Memory unit maintainer, clear the load/store queue automatically, iterates infinitly in a thread.
};

void init_main_mem();

#endif
