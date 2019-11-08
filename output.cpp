#include "output.h"

extern vector<int*> clk_wait_list;
extern vector<string> CPU_output_Q;
extern clk_tick sys_clk;
static int next_vdd;
static pthread_t handle;

class ROBEntry;

void instr_timeline_output(ROBEntry *R)
{
    string t = R->name;
    if (t.size()/8 < 2)
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
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        at_falling_edge(&lock, next_vdd);
        if (sys_clk.get_prog_ended())
        {
            for (auto s : CPU_output_Q)
                cout<<s<<endl;
            return nullptr;
        }
    }
}

void init_output_unit()
{
    CPU_output_Q.push_back("\n");
    CPU_output_Q.push_back("Instructions:\t\tISS\tEXE\tMEM\tWB\tCOMMMIT");
    clk_wait_list.push_back(&next_vdd);
    pthread_create(&handle, NULL, &output_automat, NULL);
}