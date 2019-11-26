#ifndef MIPS_H
#define MIPS_H

#include <unordered_map>

#define REG_NUM 32
#define MEM_LEN 256
#define Q_LEN 256
#define BTB_LEN 4

#include "clock.h"
#include "memory.h"
#include "import.h"
#include "instruction.h"
#include "registor.h"
#include "ROB.h"
#include "FU.h"
#include "output.h"
#include "issue.h"
#include "branch.h"

#endif