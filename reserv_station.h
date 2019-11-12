#ifndef RESERV_STATION_H
#define RESERV_STATION_H

using namespace std;

#include "FU.h"
#include "memory.h"

class resStation
{
    private:
        bool busy;                      //Indicates whether the rs is busy
        bool to_start;                  //If it is true, it meams the rs is still waiting for operands and need to start execution
        int dest;                       //The destination ROB entry of this instruction
        int Qj;                         //The location of Vj in ROB, -1 means Vj is read from ARF
        int Qk;                         //The location of Vk in ROB, -1 means Vk is read from ARF
        memCell rest;                   //Stores the outcome of instruction if needed
        memCell Vj;                     //Stores the operand j
        memCell Vk;                     //Stores the operand k
        LSQEntry *lsqE;                 //If it is a Load/Store unit, this member stores the load/store unit pointer
        int offset;                     //Offset, useful only in LD/SD/BNE/BEQ
        bool Rj;                        //Indicates whether operand j is ready
        bool Rk;                        //Indicates whether operand k is ready
        bool sub;                       //If this is a subtraction instr, Vk will be additive inversed
    public: 
        opCode code;                    //Operation code
        const valType type;             //Operand type, integer or float point. (not very useful, may remove it in future)
        pthread_t handle;               //The handle of thread running reserv_automat()
        int next_vdd;                   //The reserv_automat() registered vdd in clk_wait_list
        FU_Q *prnt_Q;                   //The pointer to the queue of function that this rs belongs to
        resStation(FU_Q *Q, valType t); //Constructor
        void set_code(opCode c);        //Set the operation code of this rs, useful when opCode = LD or SD
        void set_lsqE(LSQEntry* e);     //Set the pointer of LSQEntry is this rs, usful when opCode = LD or SD
        bool fill_rs(int _dest, const instr* _instr, int _Qj, int _Qk, memCell _Vj, memCell _Vk);       //Called in issue stage, fill up the reservation station            
        bool get_state();               //Get the current status of rs, (busy or not)
        void squash(int ROB_i);         //Clear if rs has any ROB_i tag surpassing ROB_i, should be called at falling edges
        void reserv_automat();          //reservation station automation
};

void* rs_thread_container(void *arg);   //Thread function is required to be [](void*)->void*, so it is just an enclosure
void init_resStation(resStation *rs);   //Initialize the reservation station

#endif