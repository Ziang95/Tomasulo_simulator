#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>

using namespace std;

typedef enum opCode
{
    ADD, ADD_D,
    SUB, SUB_D,
    ADDI,
    MUL_D,
    LD, SD,
    BNE, BEQ,
    NOP,
    ERR
}opCode;

typedef struct instr
{
    string name;
    opCode code;
    string dest;
    string oprnd1;
    string oprnd2 ;
    int imdt = 0;
    int offset = 0;
}instr;

#include "clock.h"

class instr_queue
{
    private:
        int head;
    public:
        bool squash;
        const size_t size;
        const vector<instr> Q;
        instr_queue(vector<instr> a);
        void move_ptr(int target);
        bool ptr_advance();
        bool ptr_branch(int offset);
        bool finished();
        int get_head();
        const instr *getInstr();
};

#endif