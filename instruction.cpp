#include "instruction.h"

instr_queue::instr_queue(vector<instr> a):
Q(a),size(a.size())
{
    squash = false;
    head = 0; 
}

bool instr_queue::finished()
{
    return head == size;
}

void instr_queue::move_ptr(int target)
{
    head = target;
}

bool instr_queue::ptr_advance()
{
    if (head >= size)
    {
        err_log("Instr queue acccess out of range while advancing");
        return false;
    }
    head ++;
    return true;
}

bool instr_queue::ptr_branch(int offset)
{
    int dest = offset + head + 1;
    if (dest >= size || dest < 0)
    {
        err_log("Instr queue acccess out of range while branching");
        return false;
    }
    head = dest;
    return true;
}

int instr_queue::get_head()
{
    return head;
}

const instr* instr_queue::getInstr()
{
    if (head == size)
        return nullptr;
    return &Q[head];
}