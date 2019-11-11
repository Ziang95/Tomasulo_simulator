#ifndef BRANCH_H
#define BRANCH_H

typedef struct BTBEntry
{
    int target;
    bool predicted;
    bool taken;
}BTBEntry;

#include "mips.h"

class BTB
{
    private:
        BTBEntry buf[BTB_LEN];
    public:
        BTB();
        void addEntry(int _instr_i, int _target);
        BTBEntry *getEntry(int instr_i);
};

class branchCtrl
{
    private:
        int ROB_i;
    public:
        pthread_t handle;
        int next_vdd;
        branchCtrl();
        void to_squash(int _ROB_i);
        void branch_automat();
};

void init_brcUnit();

#endif