#ifndef IMPORT_H
#define IMPORT_H

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#include "mips.h"
#include "instruction.h"

using namespace std;

class instr_param
{
    public:
        const int rs_num;
        const int exe_time;
        const int mem_time;
        const int fu_num;
        instr_param(int a, int b, int c, int d):
            rs_num(a),
            exe_time(b),
            mem_time(c),
            fu_num(d){};
};

class config
{
	public:
		const int* ROB_num;
		const instr_param* int_add;
		const instr_param* fp_add;
		const instr_param* fp_mul;
		const instr_param* ld_str;
		config()
        {
            ROB_num = nullptr;
            int_add = nullptr;
            fp_add = nullptr;
            fp_mul = nullptr;
            ld_str = nullptr;
        };
        ~config()
        {
            if (ROB_num) delete ROB_num;
            if (int_add) delete int_add;
            if (fp_add) delete fp_add;
            if (fp_mul) delete fp_mul;
            if (ld_str) delete ld_str;
        };
};

bool read_config_instrs(string path);

#endif
