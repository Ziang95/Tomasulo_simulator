#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef void * (*threadFunc)(void *);
typedef enum valType{INTGR,FLTP}valType;
typedef enum memOpCode{STORE, LOAD}memOpCode;
typedef union memCell
{
    int i;
    float f;
}memCell;

#include "mips.h"

typedef struct QEntry
{
    bool done;                          //Indicates whether the entry operation is finished
    memOpCode code;                     //Enum type, LOAD or STORE
    valType type;                       //Value type can either be float 
    void* val;                          //Pointer of the value to store FROM of to load TO, can be int* or float*
    int addr;                           //Address to store to or load FROM
    cond_t token;                       //When this queue entry is finished, it broadcasts its status to all threads waiting outside
}QEntry;

class memory
{
    private:
        memCell *buf;                   //Memory storage buffer, can store either Integer or Float Point value
        int size = 0;                   //Size of [buf]
        struct
        {
            valType type;               //Type of the value forwarding on CDB
            memCell val;                //The value forwarding on CDB
            int addr;                   //Where the current forwarding value comes from
        }CDB;                           //Common Data Bus
        struct QEntry LSQ[512];         //Load/Store queue
        int front, rear;                //Control pointers of circular queue [LSQ]
        bool store(QEntry& entry);      //Store the value to the address saved in LSQ entry
        bool load(QEntry& entry);       //Load the value from address saved in LSQ entry
    public:
            double value;
        pthread_t handle;               //The handle of thread running mem_automat()
        memory(int sz);                 //Constructor
        ~memory();                      //Destructor 
        bool setMem(valType type, int addr, void* val);
        QEntry* enQ(memOpCode code, valType type, int addr, void* val); //Enqueue function, Should only be called at falling edges
        void mem_automat();             //Memory unit maintainer, clear the load/store queue automatically, iterates infinitly in a thread.
};

void init_main_mem();

#endif
