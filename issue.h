#ifndef ISSUE_H
#define ISSUE_H

#include "mips.h"

void get_reg_or_rob(string regName, int &Q, memCell &V);
void *issue_automat(void *arg);
void init_issue_unit();

#endif