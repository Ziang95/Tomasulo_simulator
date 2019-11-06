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

bool resStation::fill_rs(int _dest, int _Qj, int _Qk, void *_Vj, void *_Vk)
{
    busy = true;
    dest = _dest;
    Qj = _Qj;
    Qk = _Qk;
    if (type == FLTP)
    {
        Vj.f = *(float*)_Vj;
        Vk.f = *(float*)_Vk;
    }
    else
    {
        Vj.i = *(int*)_Vj;
        Vk.i = *(int*)_Vk;
    }
    Rj = Qj<0? true:false;
    Rk = Qk<0? true:false;
    to_start = Rj&&Rk? false:true;
    if (!to_start)
        prnt_Q->enQ(dest, &rest, &Vj, &Vk);
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
            if (fCDB.get_source() == dest)
            {
                msg_log("WriteBack, ", 3);
                busy = false;
                ROBEntry *R = CPU_ROB->get_entry(dest);
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
            prnt_Q->enQ(dest, &rest, &Vj, &Vk);
        }
    }
}

void* rs_thread_container(void *arg)
{
    auto p = (resStation*) arg;
    p->reserv_automat();
    return nullptr;
}