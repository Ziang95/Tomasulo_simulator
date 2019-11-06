#include "issue.h"

extern pthread_t iss_U;
extern clk_tick sys_clk;
extern vector<int*> clk_wait_list;
extern vector<intAdder*> iAdder;
extern vector<intAdder*> fAdder;
extern vector<intAdder*> fMtplr;
extern registor reg;
extern ROB* CPU_ROB;
extern unordered_map <string, int> RAT;
extern instr_queue *instr_Q;

static int issue_next_vdd = 0;

void *issue_automat(void *arg)
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    const instr *tmp;
    int i_Vj, i_Vk;
    float f_Vj, f_Vk;
    int Qj, Qk;
    while (true)
    {
        at_rising_edge(&lock, issue_next_vdd);
        tmp = nullptr;
        Qj = Qk = -1;
        if (!instr_Q->finished())
        {
            tmp = instr_Q->getInstr();
            instr_Q->ptr_advance();
        }
        at_falling_edge(&lock, issue_next_vdd);
        if (tmp)
        {
            msg_log("Issuing code: " + tmp->name, 3);
            switch (tmp->code)
            {
            case ADD:
            {
                resStation *avai = nullptr;
                for (int i = 0; i < iAdder.size(); i++)
                {
                    for (int j = 0; j < (*iAdder[i]).rs.size(); j++)
                    {
                        if ((*iAdder[i]).rs[j]->get_state() == false)
                            avai = (*iAdder[i]).rs[j];
                    }
                }
                if (avai)
                {
                    auto found_j = RAT.find(tmp->oprnd1);
                    auto found_k = RAT.find(tmp->oprnd2);
                    if (found_j != RAT.end())
                        Qj = found_j->second;
                    else
                        reg.get(tmp->oprnd1, &i_Vj);
                    if (found_k != RAT.end())
                        Qk = found_k->second;
                    else
                        reg.get(tmp->oprnd2, &i_Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                    }            
                    RAT[tmp->dest] = dest;
                    avai->fill_rs(dest, Qj, Qk, &i_Vj, &i_Vk);
                }
            }
            default:
                break;
            }
        }
    }
    return nullptr;
}

void init_issue_unit()
{
    clk_wait_list.push_back(&issue_next_vdd);
    pthread_create(&iss_U, NULL, &issue_automat, NULL);
}