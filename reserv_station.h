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
        int offset;
        bool Rj;
        bool Rk;
        bool sub;
    public:
        opCode code;
        const valType type;
        valType retType;
        pthread_t handle;
        int next_vdd;
        FU_Q *prnt_Q;
        resStation(FU_Q *Q, valType t);
        void set_code(opCode c);
        memCell get_res();
        void set_ret_type(valType rt);
        void set_rest(memCell res);
        bool fill_rs(int _dest, int _Qj, int _Qk, memCell _Vj, memCell _Vk, int _offset, bool _sub);
        bool get_state();
        void reserv_automat();
};

class ldRes : resStation
{
    public:
        void reserv_automat();
};

void* rs_thread_container(void *arg);
void init_resStation(resStation *rs);

#endif