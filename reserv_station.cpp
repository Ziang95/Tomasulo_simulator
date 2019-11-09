#include "reserv_station.h"
#include "mips.h"

extern config *CPU_cfg;
extern clk_tick sys_clk;
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

memCell resStation::get_res()
{
    return rest;
}

void resStation::set_ret_type(valType rt)
{
    retType = rt;
}

void resStation::set_rest(memCell res)
{
    rest = res;
}

bool resStation::fill_rs(int _dest, int _Qj, int _Qk, memCell _Vj, memCell _Vk, int _offset, bool _sub)
{
    sub = _sub;
    busy = true;
    dest = _dest;
    Qj = _Qj;
    Qk = _Qk;
    offset = _offset;
    Vj = _Vj;
    Vk = _Vk;
    Rj = Qj<0? true:false;
    Rk = Qk<0? true:false;
    to_start = Rj&&Rk? false:true;
    if (!to_start)
        prnt_Q->enQ(code, retType, dest, &rest, &Vj, &Vk, offset, &busy);
    return true;
}

bool resStation::get_state()
{
    return busy;
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
                msg_log("WriteBack, ", 3);
                busy = false;
                at_falling_edge(next_vdd);
                ROBEntry *R = CPU_ROB->get_entry(dest);
                fCDB.get_val(&R->value);
                R->finished = true;
                R->output.wBack = sys_clk.get_prog_cyc();
                goto A;
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
A:      if (busy && to_start && Rj && Rk)
        {
            msg_log("Operands ready, sending to FU, dest ROB = " + to_string(dest), 3);
            to_start = false;
            if (type == FLTP)
                Vk.f = sub? -Vk.f : Vk.f;
            else
                Vk.i = sub? -Vk.i : Vk.i;
            prnt_Q->enQ(code, retType, dest, &rest, &Vj, &Vk, offset, &busy);
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
    int ret = 1;
    while(ret) ret = pthread_create(&(rs->handle), NULL, &rs_thread_container, (void*)rs);
    clk_wait_list.push_back(&rs->next_vdd);
}