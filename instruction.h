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
        const size_t size;
        const vector<instr> Q;
        instr_queue(vector<instr> a);
        bool ptr_advance();
        bool ptr_branch(int offset);
        bool finished();
        const instr *getInstr();
};

#endif