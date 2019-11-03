#ifndef RESERV_STATION_H
#define RESERV_STATION_H

using namespace std;

#include "memory.h"

typedef struct
{
    valType type;
    memCell value;
    int source;
    cond_t token;
}CDB_QEntry;

#include "mips.h"

class reservCDB{
    private:
        CDB_QEntry queue[128];
        int front, rear;
    public:
        reservCDB();
        pthread_t handle;
        bool enQ(valType t, void *v, int s);
        int get_source();
        bool get_val(void *v);
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
        virtual void exe();
        cond_t token;
};

#endif