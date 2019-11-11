#include "branch.h"
#include "clock.h"

extern unordered_map <string, int> RAT;
extern vector<int*> clk_wait_list;
extern branchCtrl brcUnit;
extern instr_queue *instr_Q;
extern ROB *CPU_ROB;
extern memory main_mem;
extern FU_CDB fCDB;
extern vector<intAdder*> iAdder;
extern vector<flpAdder*> fAdder;
extern vector<flpMtplr*> fMtplr;
extern vector<ldsdUnit*> lsUnit;

BTB::BTB()
{
    for (auto b : buf)
        b.predicted = false;
}

void BTB::addEntry(int _instr_i, int _target)
{
    int index = _instr_i%256;
    buf[index].target = _target;
    buf[index].predicted = true;
    buf[index].taken = true;
}

BTBEntry* BTB::getEntry(int _instr_i)
{
    int index = _instr_i%256;
    if (buf[index].predicted)
        return &buf[index];
    else
        return nullptr;
}

branchCtrl::branchCtrl()
{
    ROB_i = -1;
}

void branchCtrl::to_squash(int _ROB_i)
{
    ROB_i = _ROB_i;
}

void branchCtrl::branch_automat()
{
    next_vdd = 0;
    while (true)
    {
        at_rising_edge(next_vdd);
        int i = ROB_i;
        ROB_i = -1;
        if (i > -1)
        {
            int R_f = CPU_ROB->get_front();
            int R_r = CPU_ROB->get_rear();
            ROBEntry *R = CPU_ROB->get_entry(i);
            RAT = R->bkupRAT;
            main_mem.squash(i);
            at_falling_edge(next_vdd);
            fCDB.squash(i);
            for (auto fu : iAdder)
                fu->squash(i);
            for (auto fu : fAdder)
                fu->squash(i);
            for (auto fu : fMtplr)
                fu->squash(i);
            for (auto fu : lsUnit)
                fu->squash(i);
            R->finished = true;
            at_rising_edge(next_vdd);
            CPU_ROB->squash(i);
            instr_Q->squash = false;
            msg_log("squash finished", 3);
        }
        at_falling_edge(next_vdd);
    }
}

void init_brcUnit()
{
    while(pthread_create(&brcUnit.handle, NULL, [](void *arg)->void*{brcUnit.branch_automat(); return nullptr;}, NULL));
    clk_wait_list.push_back(&brcUnit.next_vdd);
}