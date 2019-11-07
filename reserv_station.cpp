#include "reserv_station.h"
#include "mips.h"

extern clk_tick sys_clk;
extern vector<int*> clk_wait_list;
extern FU_CDB fCDB;
extern ROB *CPU_ROB;

resStation::resStation(FU_Q *Q, valType t):type(t)
{
    next_vdd = 0;
    prnt_Q = Q;
    busy = false;
    dest = -1;
    Rj = Rk = true;
}

void resStation::set_code(opCode c)
{
    code = c;
}

void resStation::set_ret_type(valType rt)
{
    retType = rt;
}

void resStation::set_rest(memCell res)
{
    rest = res;
}

bool resStation::fill_rs(int _dest, int _Qj, int _Qk, void *_Vj, void *_Vk, int _offset, bool _sub)
{
    sub = _sub;
    busy = true;
    dest = _dest;
    Qj = _Qj;
    Qk = _Qk;
    offset = _offset;
    if (type == FLTP)
    {
        Vj.f = *(float*)_Vj;
        Vk.f = sub?-*(float*)_Vk:*(float*)_Vk;
    }
    else
    {
        Vj.i = *(int*)_Vj;
        Vk.i = sub?-*(int*)_Vk:*(int*)_Vk;
    }
    Rj = Qj<0? true:false;
    Rk = Qk<0? true:false;
    to_start = Rj&&Rk? false:true;
    if (!to_start)
        prnt_Q->enQ(code, retType, dest, &rest, &Vj, &Vk, offset);
    return true;
}

bool resStation::get_state()
{
    return busy;
}

void resStation::reserv_automat()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (busy)
        {
            if (code == SD)
            {
                ROBEntry *R = CPU_ROB->get_entry(dest);
                R->finished = true;
                // R->output.wBack = sys_clk.get_prog_cyc();
            }
            if (fCDB.get_source() == dest)
            {
                msg_log("WriteBack, ", 3);
                busy = false;
                ROBEntry *R = CPU_ROB->get_entry(dest);
                fCDB.get_val(&R->value);
                R->finished = true;
                R->output.wBack = sys_clk.get_prog_cyc();
            }
            else if (!Rj || !Rk)
            {
                if (fCDB.get_source() == Qj)
                {
                    fCDB.get_val(&Vj);
                    Rj = true;
                }
                else if (fCDB.get_source() == Qk)
                {
                    fCDB.get_val(&Vk);
                    Rk = true;
                }
            }
        }
        at_falling_edge(&lock, next_vdd);
        if (busy && to_start && Rj && Rk)
        {
            msg_log("Operands ready, sending to FU", 3);
            to_start = false;
            if (type == FLTP)
                Vk.f = sub? -Vk.f : Vk.f;
            else
                Vk.i = sub? -Vk.i : Vk.i;
            prnt_Q->enQ(code, retType, dest, &rest, &Vj, &Vk, offset);
        }
    }
}

void* rs_thread_container(void *arg)
{
    auto p = (resStation*) arg;
    p->reserv_automat();
    return nullptr;
}