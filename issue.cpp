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

static int next_vdd;

void get_reg_or_rob(string regName, int &Q, memCell &V)
{
    auto found = RAT.find(regName);
    if (found != RAT.end())
    {
        int tmp = found->second;
        ROBEntry *R = CPU_ROB->get_entry(tmp);
        if (R->wrtnBack)
            V = R->value;
        else
            Q = tmp;
    }
    else
        reg.get(regName, V);
}

void *issue_automat(void *arg)
{
    next_vdd = 0;
    const instr *tmp = nullptr;
    memCell Vj, Vk;
    int Qj, Qk;
    while (true)
    {
        at_rising_edge(next_vdd);
        tmp = nullptr;
        Qj = Qk = -1;
        if (!instr_Q->finished())
            tmp = instr_Q->getInstr();
        at_falling_edge(next_vdd);
        if (tmp)
        {
            msg_log("Issuing code: " + tmp->name, 2);
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
                    get_reg_or_rob(tmp->oprnd1, Qj, Vj);
                    if (tmp->code == ADDI)
                        Vk.i = tmp->imdt;
                    else
                        get_reg_or_rob(tmp->oprnd2, Qk, Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest, tmp->code);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, Qj, Qk, Vj, Vk, tmp->offset, tmp->code == SUB);
                }
                else
                    msg_log("No available iAdder rs, wait until next cycle", 2);
                
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
                    get_reg_or_rob(tmp->oprnd1, Qj, Vj);
                    get_reg_or_rob(tmp->oprnd2, Qk, Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest, tmp->code);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, Qj, Qk, Vj, Vk, tmp->offset, tmp->code == SUB_D);
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
                    get_reg_or_rob(tmp->oprnd1, Qj, Vj);
                    get_reg_or_rob(tmp->oprnd2, Qk, Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest, tmp->code);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, Qj, Qk, Vj, Vk, tmp->offset, false);
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
                    get_reg_or_rob(tmp->oprnd1, Qj, Vj);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, tmp->dest, tmp->code);
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
                    avai->fill_rs(dest, Qj, Qk, Vj, Vk, tmp->offset, false);
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
                    get_reg_or_rob(tmp->oprnd1, Qj, Vj);
                    get_reg_or_rob(tmp->dest, Qk, Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, "NO_RET", tmp->code);
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
                    avai->fill_rs(dest, Qj, Qk, Vj, Vk, tmp->offset, false);
                }
                break;
            }
            case BNE: case BEQ:
            {
                for (int i = 0; i < iAdder.size(); i++)
                    for (int j = 0; j < (*iAdder[i]).rs.size(); j++)
                        if ((*iAdder[i]).rs[j]->get_state() == false)
                            avai = (*iAdder[i]).rs[j];
                if (avai)
                {
                    get_reg_or_rob(tmp->oprnd1, Qj, Vj);
                    get_reg_or_rob(tmp->oprnd2, Qk, Vk);
                    int dest;
                    try
                    {
                        dest = CPU_ROB->add_entry(tmp->name, "NO_RET", tmp->code);
                    }
                    catch(const int e)
                    {
                        err_log("ROB is full");
                        break;
                    }
                    avai->set_ret_type(INTGR);
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, Qj, Qk, Vj, Vk, tmp->offset, false);
                    while (avai->get_state())
                    {
                        at_rising_edge(next_vdd);
                        at_falling_edge(next_vdd);
                    }
                    memCell outcome = avai->get_res();
                    if (outcome.i)
                        instr_Q->ptr_branch(tmp->offset);
                    else
                        instr_Q->ptr_advance();
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
    int ret = 1;
    while(ret) ret = pthread_create(&iss_unit, NULL, &issue_automat, NULL);
    clk_wait_list.push_back(&next_vdd);
}
