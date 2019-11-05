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
        memCell rest;
        int Qj;
        int Qk;
        memCell Vj;
        memCell Vk;
        bool Rj;
        bool Rk;
    public:
        const valType type;
        int next_vdd;
        FU_Q *prnt_Q;
        resStation(FU_Q *Q, valType t);
        bool fill_rs(int _dest, int _Qj, int _Qk, void *_Vj, void *_Vk);
        bool get_state();
        void reserv_automat();
};

#endif