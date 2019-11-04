#ifndef RESERV_STATION_H
#define RESERV_STATION_H

using namespace std;

#include "memory.h"

#include "mips.h"

class reservCDB{
    private:
        CDB queue[Q_LEN];
        CDB bus;
        int front, rear;
    public:
        int next_vdd;
        reservCDB();
        pthread_t handle;
        bool enQ(valType t, void *v, int s);
        int get_source();                      //Called at rising edges
        bool get_val(void *v);                 //Called at rising edges
        void CDB_automat();
};

class resStation
{
    private:
        bool busy;
        int dest;
        int Qj;
        int Qk;
        void *Vj;
        void *Vk;
        bool Rj;
        bool Rk;
    public:
        resStation(opCode code);
        const opCode code;
        bool fill_rs(int dest, int Qj, int Qk, void *Vj, void *Vk);
        bool get_state();
        void exe();
        cond_t token;
};

void init_reserv_CDB();

#endif