#ifndef ISSUE_H
#define ISSUE_H

#include "mips.h"

void get_reg_or_rob(string regName, int &Q, memCell &V);    //Find registor value from either ROB or ARF
void *issue_automat(void *arg);                             //Issue unit automation
void init_issue_unit();                                     //Initialize issue unit

#endif