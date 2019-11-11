#include "output.h"

using namespace std;

extern vector<int*> clk_wait_list;
extern vector<string> CPU_output_Q;
extern clk_tick sys_clk;
extern registor *reg;
extern memory main_mem;

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
            memCell m;
            cout<<"INT_REG:\t\t\t\t\tFLP_REG:"<<endl;
            for (int i = 0; i<REG_NUM/2; i++)
            {
                stringstream tmp;
                reg->get("R"+to_string(2*i), m);
                cout<<setw(3)<<"R"+to_string(2*i)<<"="<<m.i<<(m.i>9999?"\t":"\t\t");
                if (2*i + 1 < REG_NUM)
                {
                    reg->get("R"+to_string(2*i), m);
                    cout<<setw(3)<<"R"+to_string(i*2+1)<<"="<<m.i<<(m.i>9999?"\t":"\t\t");
                }
                reg->get("F"+to_string(2*i), m);
                cout<<"\t"<<setw(3)<<"F"+to_string(2*i)<<"="<<fixed<<setprecision(2)<<m.f<<"\t";
                if (2*i + 1 < REG_NUM)
                {
                    reg->get("F"+to_string(2*i+1), m);
                    cout<<setw(3)<<"F"+to_string(2*i+1)<<"="<<fixed<<setprecision(2)<<m.f<<"\t";
                }
                cout<<endl;
            }
            cout<<endl;
            for (int i = 0; i<MEM_LEN; i++)
            {
                if (abs(main_mem.getMem(i).f) > 0.001)
                    cout<<"Mem["<<i<<"]="<<fixed<<setprecision(2)<<main_mem.getMem(i).f<<", ";
            }
            cout<<endl<<endl;;
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