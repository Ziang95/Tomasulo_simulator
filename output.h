#ifndef OUTPUT_H
#define OUTPUT_H

class ROBEntry;

#include "ROB.h"

void instr_timeline_output(ROBEntry *R);

#include "mips.h"

void* output_automat(void *args);
void init_output_unit();

#endif