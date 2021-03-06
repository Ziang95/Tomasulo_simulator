#include "./headers/output.h"

using namespace std;

extern vector<int*> clk_wait_list;
extern vector<output_QEntry> CPU_output_Q;
extern clk_tick sys_clk;
extern registor *reg;
extern memory main_mem;
extern mutex_t cout_lock;

static int next_vdd = 0;
static pthread_t handle;

class ROBEntry;

void instr_timeline_output(ROBEntry *R)
{
    output_QEntry t;
    t.iss_cyc = R->output.issue;
    t.name = R->name;
    if (t.name.size()/8 < 1)
        t.name += "\t\t";
    else if (t.name.size()/8 < 2)
        t.name += "\t";
    t.name += "\t" + to_string(R->output.issue);
    t.name += "\t" + (R->output.exe > 0? to_string(R->output.exe):" ");
    t.name += "\t" + (R->output.mem > 0? to_string(R->output.mem):" ");
    t.name += "\t" + (R->output.wBack > 0? to_string(R->output.wBack):" ");
    t.name += "\t" + (R->output.commit > 0? to_string(R->output.commit):" ");
    CPU_output_Q.push_back(t);
    sort(CPU_output_Q.begin(), CPU_output_Q.end(), [](output_QEntry a, output_QEntry b){return a.iss_cyc < b.iss_cyc;});
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
            pthread_mutex_lock(&cout_lock);
            for (auto s : CPU_output_Q)
                cout<<s.name<<endl;
            cout<<endl;
            memCell m;
            cout<<"\tINT_REG:\t\t\t\tFLP_REG:"<<endl;
            for (int i = 0; i<REG_NUM/2; i++)
            {
                stringstream tmp;
                reg->get("R"+to_string(2*i), m);
                cout<<setw(3)<<"R"+to_string(2*i)<<"="<<m.i<<(m.i>9999?"\t":"\t\t");
                if (2*i + 1 < REG_NUM)
                {
                    reg->get("R"+to_string(2*i+1), m);
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
            cout<<endl<<endl;
            pthread_mutex_unlock(&cout_lock);
            return nullptr;
        }
    }
}

void init_output_unit()
{
    next_vdd = 0;
    output_QEntry tmp;
    tmp.iss_cyc = 0;
    tmp.name = "\nInstructions:\t\tISS\tEXE\tMEM\tWB\tCOMMMIT";
    CPU_output_Q.push_back(tmp);
    while(pthread_create(&handle, NULL, &output_automat, NULL));
    clk_wait_list.push_back(&next_vdd);
}