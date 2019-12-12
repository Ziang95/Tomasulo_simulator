#ifndef ROB_H
#define ROB_H

#include <iostream>
#include <string>

using namespace std;

typedef struct timeLine   //Records the start cycle of timeline issue-exe-mem-wb-commit
{
    int issue;            //Issue's start cycle
    int exe;              //Execution's start cycle
    int mem;              //Mem-stage's start cycle
    int wBack;            //Write-back's start cycle
    int commit;           //Commit's start cycle
}timeLine;

#include "mips.h"

class registor;

class ROBEntry
{
public:
    bool finished;                          //Ready to commit or not.
    bool wrtnBack;                          //Write back finished or not
    string name;                            //The full name string as it shows in the input file
    string regName;                         //Destination of the instruction (R1, R2, F1, F2)
    memCell value;                          //Result of the instruction
    opCode code;                            //Opcode of the operation.
    timeLine output;                        //Output display
    int instr_i;                            //The Correspond instruction index
    unordered_map<string, int> bkupRAT;     //A backup RAT when branch prediction is done
};

class ROB
{
    private:
        int front, rear;                    //The front and rear pointer of circular queue ROB
        ROBEntry *buf;                      //The actual buffer that stores all the ROB entries
    public:
        const int size;                     //The size of buf
        pthread_t handle;                   //The handle of thread running ROB_automat()
        int next_vdd;                       //The ROB_automat() registered Vdd in clk_wait_list
        ROB(int size);                      //Constructor
        ~ROB();                             //Destructor
        ROBEntry* get_entry(int index);     //Get the ROBEntry with index
        int add_entry(string n, string r, opCode c);    //Add an entry to ROB and return the index
        void ptr_advance();                 //Move the ROB front to the next entry
        int get_front();                    //Get the front index
        int get_rear();                     //Get the rear index
        void squash(int ROB_i);             //Rising edges
        void ROB_automate();                //ROB automation
};

void* ROB_thread_container(void* arg);      //Thread function is required to be [](void*)->void*, this is an enclosure
void init_CPU_ROB();                        //Initialize the ROB

#endif