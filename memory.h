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
    memCell value;
    int source;
}CDB;

#include "instruction.h"

typedef struct LSQEntry
{
    int rob_i;
    int SD_source;
    opCode code;                        //Enum type, should be LOAD or STORE
    memCell val;                          //Pointer of the value to store FROM of to load TO, can be int* or float*
    int addr;                           //Address to store to or load FROM
    bool ready;
}LSQEntry;

#include "mips.h"

class memory
{
    private:
        memCell *buf;                   //Memory storage buffer, can store either Integer or Float Point value
        int size;                       //Size of [buf]
        int Lfront, Lrear;              //Control pointers of circular queue [Load_Q]
        int Sfront, Srear;              //Control pointers of circular queue [Stor_Q]
        bool store(LSQEntry& entry);    //Store the value to the address saved in Load_Q entry
        bool load(LSQEntry& entry);     //Load the value from address saved in Stor_Q entry
        mutex_t LDQ_lock;               //Prevent race condition in Q modification
        mutex_t SDQ_lock;
    public:
        int mem_next_vdd;               //The mem_automat() registered vdd in sys_clk
        int SDQ_next_vdd;               //The SDQ_automat() registered vdd in sys_clk
        struct LSQEntry Load_Q[Q_LEN];  //Load queue
        struct LSQEntry Stor_Q[Q_LEN];  //Store queue
        pthread_t mem_handle;           //The handle of thread running mem_automat()
        pthread_t SDQ_handle;           //The handle of thread running SDQ_automat()
        memory(int sz);                 //Constructor
        ~memory();                      //Destructor
        bool setMem(valType type, int addr, void* val);             //Set initial mem values
        memCell getMem(int addr);
        LSQEntry* LD_enQ(int rbi, int addr);                        //Enqueue Load_Q, should only be called at falling edges
        LSQEntry* SD_enQ(int rbi, int Qj, int addr, memCell val);   //Enqueue Stor_Q, should only be called at falling edges
        void squash(int ROB_i);
        void SD_Q_automat();            //Stor_Q maintainer, can be taken as resStation for memory unit
        void mem_automat();             //Memory unit maintainer, clear the load/store queue automatically, iterates infinitly in a thread.
};

bool is_prev_index(int i, int j, int front, int rear);
void init_main_mem();

#endif
