#ifndef BRANCH_H
#define BRANCH_H

typedef struct BTBEntry
{
    int target;
    bool predicted;
    bool taken;
}BTBEntry;

typedef struct squash_param
{
    int R_f;                    //The front pointer of ROB
    int R_r;                    //The rear pointer of ROB
    int ROB_i;                  //The affected ROB index
    bool flag = false;          //Whether the squash has begun
}squash_param;

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
        int target;
    public:
        pthread_t handle;
        int next_vdd;
        branchCtrl();
        void to_squash(int _ROB_i);
        void to_target(int _target);
        int squash_ROB_i();
        void branch_automat();
};

void init_brcUnit();

#endif