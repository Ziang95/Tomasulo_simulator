#include ".\headers\ROB.h"

extern config *CPU_cfg;
extern instr_queue *instr_Q;
extern vector<int*> clk_wait_list;
extern clk_tick sys_clk;
extern ROB* CPU_ROB;
extern vector<string> CPU_output_Q;
extern unordered_map<string, int> RAT;
extern registor *reg;

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
    if (index >= size || index < 0)
    {
        err_log("ROB access out of range");
        return nullptr;
    }
    return &buf[index];
}

void ROB::ptr_advance()
{
    front = (++front)%size;
}

int ROB::get_front()
{
    return front;
}

int ROB::get_rear()
{
    return rear;
}

int ROB::add_entry(string n, string d, opCode c)
{
    if ((rear+1)%size == front)
        throw -1;
    int index = rear;
    rear = (rear + 1)%size;
    ROBEntry *tmp = buf + index;
    (*tmp).code = c;
    (*tmp).name = n;
    (*tmp).regName = d;
    (*tmp).finished = false;
    (*tmp).wrtnBack = false;
    (*tmp).output.issue = sys_clk.get_prog_cyc();
    (*tmp).output.exe = -1;
    (*tmp).output.mem = -1;
    (*tmp).output.wBack = -1;
    (*tmp).output.commit = -1;
    return index;
}

void ROB::squash(int ROB_i)
{
    for (int i = (ROB_i + 1)%size; i != rear; i = (i+1)%size)
        instr_timeline_output(&buf[i]);
    rear = (ROB_i + 1)%size;
}

void ROB::ROB_automate()
{
    next_vdd = 0;
    while (true)
    {
        at_rising_edge(next_vdd);
        if (front != rear && buf[front].finished && buf[front].code != SD)
        {
            string rName = buf[front].regName;
            msg_log("Commit " + buf[front].name + " = " + to_string(rName[0]=='R'? buf[front].value.i : buf[front].value.f) + " ROB = " + to_string(front), 2);
            buf[front].output.commit = sys_clk.get_prog_cyc();
            if (buf[front].regName != "NO_RET")
            {
                auto got = RAT.find(rName);
                if (got != RAT.end() && got->second == front)
                    RAT.erase(got);
                reg->set(rName, buf[front].value);
            }
            instr_timeline_output(&buf[front]);
            at_falling_edge(next_vdd);
            ptr_advance();
            msg_log("ROB pointer moved to: " + to_string(front), 3);
        }
        else
        {
            if (front == rear)
                if (instr_Q->finished())
                    sys_clk.end_instr();
            else
                msg_log("ROB waiting for entry (" + to_string(front) + ") to finish", 3);
            at_falling_edge(next_vdd);
        }
    }
}

void* ROB_thread_container(void* arg)
{
    auto p = (ROB*) arg;
    p->ROB_automate();
    return nullptr;
}

void init_CPU_ROB()
{
    if (CPU_ROB)
        delete CPU_ROB;
    CPU_ROB = new ROB(*(CPU_cfg->ROB_num));
    while(pthread_create(&CPU_ROB->handle, NULL, &ROB_thread_container, CPU_ROB));
    clk_wait_list.push_back(&CPU_ROB->next_vdd);
}
