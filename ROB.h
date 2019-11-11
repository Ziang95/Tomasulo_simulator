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
    bool finished;      // Ready to commit or not.
    bool wrtnBack;      // Write back finished or not
    string name;        // The full name string as it shows in the input file
    string regName;     // Destination of the instruction (R1, R2, F1, F2)
    memCell value;      // Result of the instruction
    opCode code;        // Opcode of the operation.
    timeLine output;    // Output display
    int instr_i;        // The Correspond instruction index
    unordered_map<string, int> bkupRAT;
};

class ROB
{
    private:
        int front, rear;
        ROBEntry *buf;
    public:
        const int size;
        pthread_t handle;
        int next_vdd;
        ROB(int size);
        ~ROB();
        ROBEntry* get_entry(int index);
        int add_entry(string n, string r, opCode c);
        void ptr_advance();
        int get_front();
        int get_rear();
        void squash(int ROB_i);             //Rising edges
        void ROB_automate();
};

void* ROB_thread_container(void* arg);
void init_CPU_ROB();

#endif