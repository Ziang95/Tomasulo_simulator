#ifndef RESERV_STATION_H
#define RESERV_STATION_H

#include "mips.h"

class CDB{
    private:
        valType type;
        memCell value;
        int source;
    public:
        bool set_bus(valType type, void *val, int source);
        int get_source();
        bool get_val(void *val);
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