#include "ROB.h"

extern clk_tick sys_clk;
extern ROB* CPU_ROB;
extern unordered_map<string, int> RAT;

ROB::ROB(int s):size(s)
{
    buf = new ROBEntry[s];
    front = rear = 0;
}

ROB::~ROB()
{
    if (buf)
        delete[] buf;
    buf = nullptr;
}

ROBEntry *ROB::get_entry(int index)
{
    return &buf[index];
}

int ROB::add_entry(string n, string d)
{
    if ((rear+1)%size == front)
        throw -1;
    int index = rear;
    rear = (rear + 1)%size;
    buf[index].name = n;
    buf[index].regName = d;
    buf[index].finished = false;
    buf[index].output.issue = sys_clk.get_prog_cyc();
    buf[index].output.mem = -1;
    return index;
}

void ROB::ROB_automate()
{
    mutex_t lock;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (front != rear && buf[front].finished)
        {
            buf[front].output.commit = sys_clk.get_prog_cyc();
            auto got = RAT.find(buf[front].regName);
            if (got != RAT.end() && got->second == front)
                RAT.erase(got);
            //out put table;
            front = (++front)%size;
        }
        at_falling_edge(&lock, next_vdd);
    }
}