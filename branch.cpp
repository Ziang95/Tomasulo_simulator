#include "./headers/branch.h"
#include "./headers/clock.h"

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
extern nopBublr* nopBub;

extern cond_t squash_cond;
extern mutex_t squash_mutex;

BTB::BTB()
{
    for (auto b : buf)
        b.predicted = false;
}

void BTB::addEntry(int _instr_i, int _target)
{
    int index = _instr_i%BTB_LEN;
    buf[index].target = _target;
    buf[index].predicted = true;
    buf[index].taken = true;
}

BTBEntry* BTB::getEntry(int _instr_i)
{
    int index = _instr_i%BTB_LEN;
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

void branchCtrl::to_target(int _target)
{
    target = _target;
}

int branchCtrl::squash_ROB_i()
{
    return ROB_i;
}

void branchCtrl::branch_automat()
{
    next_vdd = 0;
    while (true)
    {
        at_rising_edge(next_vdd);
        at_falling_edge(next_vdd);
        if (ROB_i>-1)
        {
            pthread_mutex_lock(&squash_mutex);
            while (!instr_Q->squash)
                pthread_cond_wait(&squash_cond, &squash_mutex);
            pthread_mutex_unlock(&squash_mutex);
            ROBEntry *R = CPU_ROB->get_entry(ROB_i);
            RAT = R->bkupRAT;
            at_falling_edge(next_vdd);
            for (auto fu : iAdder)
                fu->squash(ROB_i);
            for (auto fu : fAdder)
                fu->squash(ROB_i);
            for (auto fu : fMtplr)
                fu->squash(ROB_i);
            for (auto fu : lsUnit)
                fu->squash(ROB_i);
            nopBub->squash(ROB_i);
            main_mem.squash(ROB_i);
            fCDB.squash(ROB_i);
            R->finished = true;
            CPU_ROB->squash(ROB_i);
            at_rising_edge(next_vdd);
            instr_Q->move_ptr(target);
            msg_log("squash finished", 3);
            at_falling_edge(next_vdd);
            at_rising_edge(next_vdd);
            instr_Q->squash = false;
            ROB_i = -1;
            at_falling_edge(next_vdd);
        }
    }
}

void init_brcUnit()
{
    while(pthread_create(&brcUnit.handle, NULL, [](void *arg)->void*{brcUnit.branch_automat(); return nullptr;}, NULL));
    clk_wait_list.push_back(&brcUnit.next_vdd);
}