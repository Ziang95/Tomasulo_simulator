#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef enum valType{INTGR,FLTP}valType;        //The value type can be INT or FLP, but not very useful actually, consider removing it
typedef union memCell                           //Since every variable in CPU is either integer or float point, so it is a union type
{
    int i;
    float f;
}memCell;
typedef struct CDB                              //Common data bus should have both value and source ROB index
{
    memCell value;
    int source;
}CDB;

#include "instruction.h"

typedef struct LSQEntry
{
    int rob_i;                          //The index of LD/SD instr in ROB
    int SD_source;                      //The location of value to store in ROB, -1 means it comes from ARF
    opCode code;                        //Enum type, should be LD or SD
    memCell val;                        //The value to store (not useful in LD)
    int addr;                           //Address to store to or load from
    bool ready;                         //When both addr and value is ready, the LSQEntry is ready to load/store
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
        mutex_t LDQ_lock;               //Prevent race condition in Load_Q modification
        mutex_t SDQ_lock;               //Prevent race condition in Stor_Q modification
    public:
        int mem_next_vdd;               //The mem_automat() registered vdd in clk_wait_list
        int SDQ_next_vdd;               //The SDQ_automat() registered vdd in clk_wait_list
        struct LSQEntry Load_Q[Q_LEN];  //Load queue
        struct LSQEntry Stor_Q[Q_LEN];  //Store queue, can be viewed as the reservation station of memory unit
        pthread_t mem_handle;           //The handle of thread running mem_automat()
        pthread_t SDQ_handle;           //The handle of thread running SDQ_automat()
        memory(int sz);                 //Constructor
        ~memory();                      //Destructor
        bool setMem(valType type, int addr, void* val);             //Set initial mem values before program starts
        memCell getMem(int addr);                                   //Get the stored value after program ends
        LSQEntry* LD_enQ(int rbi, int addr);                        //Enqueue Load_Q, should only be called at falling edges
        LSQEntry* SD_enQ(int rbi, int Qj, int addr, memCell val);   //Enqueue Stor_Q, should only be called at falling edges
        void squash(int ROB_i);                                     //Clear all the entries surpassing ROB_i in Load/Store queue, should be called at falling edges
        void SD_Q_automat();            //Stor_Q maintainer
        void mem_automat();             //Memory unit maintainer, clear the load/store queue automatically, iterates infinitly in a thread.
};

bool is_prev_index(int i, int j, int front, int rear);              //Given the [front] and [rear] of a circular queue, it tells whether i is before j
void init_main_mem();                                               //Initialize the memory unit

#endif
