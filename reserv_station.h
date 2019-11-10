#ifndef RESERV_STATION_H
#define RESERV_STATION_H

using namespace std;

#include "FU.h"
#include "memory.h"

class resStation
{
    private:
        bool busy;
        bool to_start;
        int dest;
        int Qj;
        int Qk;
        memCell rest;
        memCell Vj;
        memCell Vk;
        LSQEntry *lsqE;
        int offset;
        bool Rj;
        bool Rk;
        bool sub;
    public:
        opCode code;
        const valType type;
        pthread_t handle;
        int next_vdd;
        FU_Q *prnt_Q;
        resStation(FU_Q *Q, valType t);
        void set_code(opCode c);
        void set_lsqE(LSQEntry* e);
        void set_rest(memCell res);
        bool fill_rs(int _dest, const instr* _instr, int _Qj, int _Qk, memCell _Vj, memCell _Vk);
        memCell get_res();
        bool get_state();
        void reserv_automat();
};

void* rs_thread_container(void *arg);
void init_resStation(resStation *rs);

#endif