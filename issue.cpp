#include ".\headers\issue.h"

extern pthread_t iss_unit;
extern clk_tick sys_clk;
extern ROB* CPU_ROB;
extern registor *reg;
extern unordered_map <string, int> RAT;
extern instr_queue *instr_Q;
extern BTB CPU_BTB;
extern vector<intAdder*> iAdder;
extern vector<flpAdder*> fAdder;
extern vector<flpMtplr*> fMtplr;
extern vector<ldsdUnit*> lsUnit;
extern nopBublr* nopBub;
extern branchCtrl brcUnit;
extern vector<int*> clk_wait_list;

static int next_vdd;
cond_t squash_cond = PTHREAD_COND_INITIALIZER;
mutex_t squash_mutex = PTHREAD_MUTEX_INITIALIZER;

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
        reg->get(regName, V);
}

bool check_squash()
{
    msg_log("Checking squash status", 4);
    if (brcUnit.squash_ROB_i() > -1)
    {
        pthread_mutex_lock(&squash_mutex);
        instr_Q->squash = true;
        pthread_cond_broadcast(&squash_cond);
        pthread_mutex_unlock(&squash_mutex);
        at_rising_edge(next_vdd);
        at_falling_edge(next_vdd);
        return true;
    }
    return false;
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
                    if (check_squash())
                        break;
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, tmp, Qj, Qk, Vj, Vk);
                }
                else
                {
                    msg_log("No available iAdder rs, wait until next cycle", 2);
                    check_squash();
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
                    if (check_squash())
                        break;
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, tmp, Qj, Qk, Vj, Vk);
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
                    if (check_squash())
                        break;
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, tmp, Qj, Qk, Vj, Vk);
                }
                else
                {
                    msg_log("No available fAdder rs, wait until next cycle", 2);
                    check_squash();
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
                    if (check_squash())
                        break;
                    instr_Q->ptr_advance();
                    RAT[tmp->dest] = dest;
                    avai->fill_rs(dest, tmp, Qj, Qk, Vj, Vk);
                }
                else
                {
                    msg_log("No available LD rs, wait until next cycle", 2);
                    check_squash();
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
                    if (check_squash())
                        break;
                    instr_Q->ptr_advance();
                    avai->set_code(SD);
                    avai->fill_rs(dest, tmp, Qj, Qk, Vj, Vk);
                }
                else
                {
                    msg_log("No available SD rs, wait until next cycle", 2);
                    check_squash();
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
                    if (check_squash())
                        break;
                    ROBEntry *R = CPU_ROB->get_entry(dest);
                    R->instr_i = instr_Q->get_head();
                    R->bkupRAT = RAT;
                    avai->set_code(tmp->code);
                    avai->fill_rs(dest, tmp, Qj, Qk, Vj, Vk);
                    if (BTBEntry* predctr = CPU_BTB.getEntry(R->instr_i))
                    {
                        if (predctr->taken)
                            instr_Q->move_ptr(((R->instr_i)/8)*8 + (predctr->target)%8);
                        else
                            instr_Q->ptr_advance();
                    }
                    else
                        instr_Q->ptr_advance();
                }
                else
                {
                    msg_log("No available branch rs, wait until next cycle", 2);
                    check_squash();
                }
                break;
            }
            case NOP:
            {
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
                if (check_squash())
                    break;
                instr_Q->ptr_advance();
                nopBub->generate_bubble(dest);
                break;
            }
            default:
                if (check_squash())
                    break;
                break;
            }
        }
        else
            check_squash();
    }
    return nullptr;
}

void init_issue_unit()
{
    while(pthread_create(&iss_unit, NULL, &issue_automat, NULL));
    clk_wait_list.push_back(&next_vdd);
}
