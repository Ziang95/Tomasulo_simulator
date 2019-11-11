#include "reserv_station.h"
#include "mips.h"

extern config *CPU_cfg;
extern clk_tick sys_clk;
extern memory main_mem;
extern vector<int*> clk_wait_list;
extern FU_CDB fCDB;
extern ROB *CPU_ROB;

resStation::resStation(FU_Q *Q, valType t):type(t)
{
    prnt_Q = Q;
    busy = false;
    dest = -1;
    Rj = Rk = true;
}

void resStation::set_code(opCode c)
{
    code = c;
}

void resStation::set_lsqE(LSQEntry *e)
{
    lsqE = e;
}

memCell resStation::get_res()
{
    return rest;
}

void resStation::set_rest(memCell res)
{
    rest = res;
}

bool resStation::fill_rs(int _dest_robi, const instr* _instr, int _Qj, int _Qk, memCell _Vj, memCell _Vk)
{
    sub = _instr->code == SUB || _instr->code == SUB_D;
    busy = true;
    dest = _dest_robi;
    code = _instr->code;
    Qj = _Qj;
    Qk = _Qk;
    offset = _instr->offset;
    Vj = _Vj;
    Vk = _Vk;
    Rj = Qj<0? true:false;
    Rk = Qk<0? true:false;
    to_start = Rj&&Rk? false:true;
    if (code == LD)
        lsqE = main_mem.LD_enQ(dest, -1);
    else if (code == SD)
    {
        memCell SD_source;
        int SD_source_rob = -1;
        get_reg_or_rob(_instr->dest, SD_source_rob, SD_source);
        lsqE = main_mem.SD_enQ(dest, SD_source_rob, -1, SD_source);
    }
    if (!to_start)
    {
        msg_log("Operands ready, sending to FU, dest ROB = " + to_string(dest), 2);
        prnt_Q->enQ(code, dest, &rest, &Vj, &Vk, lsqE, offset, &busy);
    }
    msg_log("Res Station filled, ROB = " + to_string(dest), 3);
    return true;
}

bool resStation::get_state()
{
    return busy;
}

void resStation::squash(int ROB_i)
{
    int R_f = CPU_ROB->get_front();
    int R_r = CPU_ROB->get_rear();
    if (is_prev_index(ROB_i, dest, R_f, R_r) || is_prev_index(ROB_i, Qj, R_f, R_r) || is_prev_index(ROB_i, Qk, R_f, R_r))
        busy = false;
}

void resStation::reserv_automat()
{
    next_vdd = 0;
    bool to_commit;
    while (true)
    {
        at_rising_edge(next_vdd);
        if (busy)
        {
            if (fCDB.get_source() == dest)
            {
                msg_log("WriteBack to ROB = " + to_string(dest), 2);
                ROBEntry *R = CPU_ROB->get_entry(dest);
                fCDB.get_val(&R->value);
                R->wrtnBack = true;
                at_falling_edge(next_vdd);
                R->finished = true;
                R->output.wBack = sys_clk.get_prog_cyc();
                at_rising_edge(next_vdd);
                busy = false;
                at_falling_edge(next_vdd);
                continue;
            }
            else if (!Rj || !Rk)
            {
                if (fCDB.get_source() == Qj)
                {
                    fCDB.get_val(&Vj);
                    Rj = true;
                }
                if (fCDB.get_source() == Qk)
                {
                    fCDB.get_val(&Vk);
                    Rk = true;
                }
            }
        }
        at_falling_edge(next_vdd);
        if (!Rj || !Rk)
        {
            msg_log("ROB = " + to_string(dest) + " waiting for operands" + (Rj?"":(" Qj = "+to_string(Qj))) + (Rk?"":(" Qk = "+to_string(Qk))), 3);
        }
        if (busy && to_start && Rj && Rk)
        {
            msg_log("Operands ready, sending to FU, dest ROB = " + to_string(dest), 2);
            to_start = false;
            if (type == FLTP)
                Vk.f = sub? -Vk.f : Vk.f;
            else
                Vk.i = sub? -Vk.i : Vk.i;
            prnt_Q->enQ(code, dest, &rest, &Vj, &Vk, lsqE, offset, &busy);
        }
    }
}

void* rs_thread_container(void *arg)
{
    auto p = (resStation*) arg;
    p->reserv_automat();
    return nullptr;
}

void init_resStation(resStation *rs)
{
    while(pthread_create(&(rs->handle), NULL, &rs_thread_container, (void*)rs));
    clk_wait_list.push_back(&rs->next_vdd);
}