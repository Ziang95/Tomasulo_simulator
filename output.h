#ifndef OUTPUT_H
#define OUTPUT_H

#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

typedef struct output_QEntry
{
    int iss_cyc;
    string name;
}output_QEnntry;

class ROBEntry;

#include "ROB.h"

void instr_timeline_output(ROBEntry *R);

#include "mips.h"

void* output_automat(void *args);
void init_output_unit();

#endif