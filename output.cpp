#include "output.h"

using namespace std;

extern vector<int*> clk_wait_list;
extern vector<string> CPU_output_Q;
extern clk_tick sys_clk;
extern registor reg;

static int next_vdd = 0;
static pthread_t handle;

class ROBEntry;

void instr_timeline_output(ROBEntry *R)
{
    string t = R->name;
    if (t.size()/8 < 1)
        t += "\t\t";
    else if (t.size()/8 < 2)
        t += "\t";
    t += "\t" + to_string(R->output.issue);
    t += "\t" + to_string(R->output.exe);
    t += "\t" + (R->output.mem > 0? to_string(R->output.mem):" ");
    t += "\t" + (R->output.wBack > 0? to_string(R->output.wBack):" ");
    t += "\t" + to_string(R->output.commit);
    CPU_output_Q.push_back(t);
}

void* output_automat(void *args)
{
    next_vdd = 0;
    while (true)
    {
        at_rising_edge(next_vdd);
        at_falling_edge(next_vdd);
        if (sys_clk.is_instr_ended() && sys_clk.is_mem_ended())
        {
            for (auto s : CPU_output_Q)
                cout<<s<<endl;
            cout<<endl;
            string regs = "";
            memCell m;
            stringstream tmp;
            for (int i = 0; i<INT_REG_NUM; i++)
            {
                reg.get("R"+to_string(i), m);
                regs += to_string(m.i) + ", ";
            }
            cout<<regs<<endl<<endl;
            for (int i = 0; i<FP_REG_NUM; i++)
            {
                reg.get("F"+to_string(i), m);
                tmp<<std::fixed<<std::setprecision(2)<<m.f;
                tmp<<", ";
            }
            cout<<tmp.str()<<endl;
            return nullptr;
        }
    }
}

void init_output_unit()
{
    next_vdd = 0;
    CPU_output_Q.push_back("\n");
    CPU_output_Q.push_back("Instructions:\t\tISS\tEXE\tMEM\tWB\tCOMMMIT");
    while(pthread_create(&handle, NULL, &output_automat, NULL));
    clk_wait_list.push_back(&next_vdd);
}