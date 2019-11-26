
#ifndef FU_H
#define FU_H

#include <vector>
using namespace std;

#include "instruction.h"
#include "memory.h"

struct LSQEntry;

typedef struct FU_QEntry
{
    opCode code;            //Operation code
    int dest;               //Destination ROB index
    memCell *res;           //Points to the result member in corresponding rs
    memCell *oprnd1;        //Points to the operand_j member in corresponding rs
    memCell *oprnd2;        //Points to the operand_k member in corresponding rs
    LSQEntry *lsqe;         //If this task is a LD/SD, this points the the Load/Store queue entry
    int offset;             //Useful in LD/SD/BNE/BEQ
    bool *busy;             //If the task doesn't need write back, the reservation station will be set idle as soon as the calculation is done
}FU_QEntry;

#include "mips.h"

class FU_CDB{
    private:
        CDB queue[Q_LEN];   //Everyone who want to use CDB need to queue up
        CDB bus;            //The current value being broadcast through this bus
        int front, rear;    //Front and rear pointer of queue
        mutex_t Q_lock;     //Prevent race condition in queue modification
    public:
        int next_vdd;       //The CDB_automat() registered Vdd in clk_wait_list
        FU_CDB();           //Constructor
        pthread_t handle;   //The handle of thread runnin CDB_automat()
        bool enQ(memCell *v, int s);    //Enqueue a value to broadcast, should be called at falling edges
        int get_source();               //Returns the source ROB index of value being broadcasted, should be called at rising edges
        bool get_val(memCell *v);       //Returns the value being broadcasted, should be called at rising edges
        void squash(int ROB_i);         //Clear all the entries surpassing ROB_i, should be called at falling edges
        void CDB_automat();             //CDB automation
};

void init_FU_CDB();

class FU_Q
{
    private:
        FU_QEntry queue[Q_LEN];         //Function unit has a priority queue of length [Q_LEN]
        int front, rear;                //Front and rear pointer of circular queue [queue]
        mutex_t Q_lock;                 //Prevent race condition in queue modification
    public:
        FU_Q();                         //Constructor
        const FU_QEntry* enQ(opCode c, int d, memCell *r, memCell *op1, memCell *op2, LSQEntry* _lsqe, int offst, bool *busy);  //Enqueue function
        void squash(int ROB_i);         //Clear all the entries surpassing ROB_i, should be called at falling edges
        FU_QEntry* deQ();               //Dequeue function;
};

class resStation;

#include "reserv_station.h"

class functionUnit
{
    public:
        FU_Q queue;                     //Each function unit has a task queue
        FU_QEntry task;                 //The current task of function unit
        vector<resStation*> rs;         //The reservation stations belonging to this function unit
        void squash(int ROB_i);         //Squash every reservation station and task queue
        int next_vdd;                   //FU_automat() registered Vdd in clk_wait_list
        pthread_t handle;               //The handle of thread running FU_automat()
        struct
        {
            int R_f;
            int R_r;
            int ROB_i;
            bool flag = false;
        }squash_info;
        virtual void FU_automat(){};    //FU automation
};

class intAdder : public functionUnit
{
    public:
        intAdder();
        ~intAdder();
        void FU_automat();
};

class flpAdder : public functionUnit
{
    private:
        FU_QEntry *pLatency_Q;                      //Since this FU is pipelined, so it can be viewed as every task is done instantly but has a latency of [exe_time]
        void shift(FU_QEntry &in, FU_QEntry &out);  //The shifter for simulating the latency
    public:
        flpAdder();
        ~flpAdder();
        void FU_automat();
};

class flpMtplr : public functionUnit
{
    private:
        FU_QEntry *pLatency_Q;                      //Since this FU is pipelined, so it can be viewed as every task is done instantly but has a latency of [exe_time]
        void shift(FU_QEntry &in, FU_QEntry &out);  //The shifter for simulating the latency
    public:
        flpMtplr();
        ~flpMtplr();
        void FU_automat();
};

class ldsdUnit: public functionUnit
{
    public:
        ldsdUnit();
        ~ldsdUnit();
        void FU_automat();
};

struct ROBEntry;

typedef struct bubbEntry
{
    int ROB_i;
    ROBEntry* ROB_E;
}bubbEntry;

class nopBublr                  //This is for NOP instruction, since NOP doesnt actually operates but occupies 1 cycle in each stage
{
    private:
        bubbEntry bubble;       //The new NOP if specified
        bubbEntry shifter[3];   //Every bubble is shifted 3 times in this shifter, in [0] it finishes EXE stage, in [1] finishes WB, in [2] it commits
    public:
        pthread_t handle;       //Handle of thread running FU_automat()
        int next_vdd;           //FU_automat() registered Vdd in clk_wait_list
        nopBublr();             //Constructor
        void squash(int ROB_i);
        void generate_bubble(int ROB_i);    //Like the fill() in reservation, starts an NOP instr
        void FU_automat();      //FU automation
};

void* intAdder_thread_container(void *arg); //Thread function is required to be [](void*)->void*, so it is just an enclosure
void* flpAdder_thread_container(void *arg); //Thread function is required to be [](void*)->void*, so it is just an enclosure
void* flpMlptr_thread_container(void *arg); //Thread function is required to be [](void*)->void*, so it is just an enclosure
void* ldsdUnit_thread_container(void *arg); //Thread function is required to be [](void*)->void*, so it is just an enclosure

void init_FUs();

#endif
