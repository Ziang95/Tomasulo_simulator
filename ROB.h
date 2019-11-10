#ifndef ROB_H
#define ROB_H

#include <iostream>
#include <string>

using namespace std;

typedef struct timeLine
{
    int issue;
    int exe;
    int mem;
    int wBack;
    int commit;
}timeLine;

#include "mips.h"

class registor;

class squashData
{
    public:
        squashData();
        registor *reg_backup;
        unordered_map<string, int> RAT_backup;
};

class ROBEntry
{
public:
    bool finished;      // Ready to commit or not.
    bool wrtnBack;
    string name;
    string regName;     // Destination of the instruction (R1, R2, F1, F2)
    memCell value;      // Result of the instruction
    opCode code;        // Opcode of the operation.
    timeLine output;

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
        void ROB_automate();
};

void* ROB_thread_container(void* arg);
void init_CPU_ROB();

#endif