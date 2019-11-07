#include "ROB.h"

extern config *CPU_cfg;
extern vector<int*> clk_wait_list;
extern clk_tick sys_clk;
extern ROB* CPU_ROB;
extern unordered_map<string, int> RAT;
extern registor reg;

ROB::ROB(int s):size(s)
{
    next_vdd = 0;
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

int ROB::get_front()
{
    return front;
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
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (front != rear && buf[front].finished)
        {
            string rName = buf[front].regName;
            msg_log("Commit " + rName + " = " + to_string(rName[0]=='R'? buf[front].value.i : buf[front].value.f), 3);
            buf[front].output.commit = sys_clk.get_prog_cyc();
            auto got = RAT.find(rName);
            if (got != RAT.end() && got->second == front)
                RAT.erase(got);
            reg.set(rName, &buf[front].value);
            //out put table;
            at_falling_edge(&lock, next_vdd);
            front = (++front)%size;
        }
        else
            at_falling_edge(&lock, next_vdd);
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
    clk_wait_list.push_back(&CPU_ROB->next_vdd);
    pthread_create(&CPU_ROB->handle, NULL, &ROB_thread_container, CPU_ROB);
}
