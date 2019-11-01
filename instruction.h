#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
using namespace std;

typedef enum opCode
{
    ADD, ADD_D,
    SUB, SUB_D,
    ADDI,
    MUL_D,
    LD, SD,
    BNE, BEQ
}opCode;

typedef struct instr
{
    string name;
    opCode code;
    string dest;
    string oprnd1;
    string oprnd2;
    int imdt;
    int offset;
}instr;

class instr_queue
{
    private:
        int head;
    public:
        const size_t size;
        const vector<instr> Q;
        instr_queue(vector<instr> a):Q(a),size(a.size())
        {
            head = 0; 
        };
        bool ptr_advance()
        {
            if (head >= size)
            {
                err_log("Instr queue acccess out of range while advancing");
                return false;
            }
            head ++;
            return true;
        };
        BOOLEAN ptr_branch(int dest)
        {
            if (dest >= size || dest<0)
            {
                err_log("Instr queue acccess out of range while branching");
                return false;
            }
            head = dest;
            return true;
        };
        const instr *getInstr();
};

#endif