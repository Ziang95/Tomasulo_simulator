#include "issue.h"

extern pthread_t iss_unit;
extern clk_tick sys_clk;
extern vector<int*> clk_wait_list;
extern vector<intAdder*> iAdder;
extern vector<flpAdder*> fAdder;
extern vector<flpMtplr*> fMtplr;
extern vector<ldsdUnit*> lsUnit;
extern registor reg;
extern ROB* CPU_ROB;
extern unordered_map <string, int> RAT;
extern instr_queue *instr_Q;

static int next_vdd = 0;

void get_int_reg_or_rob(string regName, int *Q, int *V)
{
    auto found = RAT.find(regName);
    if (found != RAT.end())
    {
        int tmp = found->second;
        ROBEntry *R = CPU_ROB->get_entry(tmp);
        if (R->finished)
            *V = R->value.i;
        else
            *Q = tmp;
    }
    else
        reg.get(regName, V);
}

void get_flp_reg_or_rob(string regName, int *Q, float *V)
{
    auto found = RAT.find(regName);
    if (found != RAT.end())
    {
        int tmp = found->second;
        ROBEntry *R = CPU_ROB->get_entry(tmp);
        if (R->finished)
            *V = R->value.f;
        else
            *Q = tmp;
    }
    else
        reg.get(regName, V);
}

void *issue_automat(void *arg)
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    const instr *tmp = nullptr;
    int i_Vj, i_Vk;
    float f_Vj, f_Vk;
    int Qj, Qk;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        tmp = nullptr;
        Qj = Qk = -1;
        if (!instr_Q->finished())
            tmp = instr_Q->getInstr();
        at_falling_edge(&lock, next_vdd);
        if (tmp)
        {
            msg_log("Issuing code: " + tmp->name, 3);
            resStation *avai = nullptr;
            switch (tmp->code)
            {
            case ADD: case ADDI: case SUB:
            {
                for (int i = 0; i < iAdder.size(); i++)
                    for (int j = 0; j < (*iAdder[i]).rs.size(); j++)
                        if ((*iAdder[i]).rs[j]->get_state() == false)
                            avai = (*iAdder[i]).rs[j];
                if (avai)
                {
                    get_int_reg_or_rob(tmp->oprnd1, &Qj, &i_Vj);
                    if (tmp->code == ADDI)
                        i_Vk = tmp->imdt;
                    else
                        get_int_reg_or_rob(tmp->oprnd2, &Qk, &i_Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->fill_rs(dest, Qj, Qk, &i_Vj, &i_Vk, tmp->offset, tmp->code == SUB);
                }
                break;
            }
            case ADD_D: case SUB_D:
            {
                for (int i = 0; i < fAdder.size(); i++)
                    for (int j = 0; j < (*fAdder[i]).rs.size(); j++)
                        if ((*fAdder[i]).rs[j]->get_state() == false)
                            avai = (*fAdder[i]).rs[j];
                if (avai)
                {
                    get_flp_reg_or_rob(tmp->oprnd1, &Qj, &f_Vj);
                    get_flp_reg_or_rob(tmp->oprnd2, &Qk, &f_Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->fill_rs(dest, Qj, Qk, &f_Vj, &f_Vk, tmp->offset, tmp->code == SUB_D);
                }
                break;
            }
            case MUL_D:
            {
                for (int i = 0; i < fMtplr.size(); i++)
                    for (int j = 0; j < (*fMtplr[i]).rs.size(); j++)
                        if ((*fMtplr[i]).rs[j]->get_state() == false)
                            avai = (*fMtplr[i]).rs[j];
                if (avai)
                {
                    get_flp_reg_or_rob(tmp->oprnd1, &Qj, &f_Vj);
                    get_flp_reg_or_rob(tmp->oprnd2, &Qk, &f_Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->fill_rs(dest, Qj, Qk, &f_Vj, &f_Vk, tmp->offset, false);
                }
                break;
            }
            case LD:
            {
                for (int i = 0; i < lsUnit.size(); i++)
                    for (int j = 0; j < (*lsUnit[i]).rs.size(); j++)
                        if ((*lsUnit[i]).rs[j]->get_state() == false)
                            avai = (*lsUnit[i]).rs[j];
                if (avai)
                {
                    get_int_reg_or_rob(tmp->oprnd1, &Qj, &i_Vj);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    if (tmp->dest[0] == 'R')
                        avai->set_ret_type(INTGR);
                    else
                        avai->set_ret_type(FLTP);
                    avai->set_code(LD);
                    avai->fill_rs(dest, Qj, Qk, &i_Vj, &i_Vk, tmp->offset, false);
                }
                break;
            }
            case SD:
            {
                for (int i = 0; i < lsUnit.size(); i++)
                    for (int j = 0; j < (*lsUnit[i]).rs.size(); j++)
                        if ((*lsUnit[i]).rs[j]->get_state() == false)
                            avai = (*lsUnit[i]).rs[j];
                if (avai)
                {
                    get_int_reg_or_rob(tmp->oprnd1, &Qj, &i_Vj);
                    get_flp_reg_or_rob(tmp->dest, &Qk, &f_Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, "SD");
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    if (tmp->dest[0] == 'R')
                        avai->set_ret_type(INTGR);
                    else
                        avai->set_ret_type(FLTP);
                    avai->set_code(SD);
                    avai->fill_rs(dest, Qj, Qk, &i_Vj, &f_Vk, tmp->offset, false);
                }
                break;
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
    clk_wait_list.push_back(&next_vdd);
    pthread_create(&iss_unit, NULL, &issue_automat, NULL);
}
