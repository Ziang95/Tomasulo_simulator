
#ifndef FU_H
#define FU_H

#include <vector>
using namespace std;

#include "instruction.h"
#include "memory.h"

struct LSQEntry;

typedef struct FU_QEntry
{
    bool done;
    opCode code;
    valType rtType;
    int dest;
    memCell *res;
    memCell *oprnd1;
    memCell *oprnd2;
    LSQEntry *lsqe;
    int offset;
    bool *busy;
}FU_QEntry;

#include "mips.h"

class FU_CDB{
    private:
        CDB queue[Q_LEN];
        CDB bus;
        int front, rear;
        mutex_t Q_lock;
    public:
        int next_vdd;
        FU_CDB();
        pthread_t handle;
        bool enQ(memCell *v, int s);                //Called at falling edges
        int get_source();                           //Called at rising edges
        bool get_val(memCell *v);                   //Called at rising edges
        void squash(int ROB_i);                     //Called at falling edges
        void CDB_automat();                         //CDB maintainer
};

void init_FU_CDB();

class FU_Q
{
    private:
        FU_QEntry queue[Q_LEN];
        int front, rear;
        mutex_t Q_lock;
    public:
        FU_Q();
        const FU_QEntry* enQ(opCode c, int d, memCell *r, memCell *op1, memCell *op2, LSQEntry* _lsqe, int offst, bool *busy);
        void squash(int ROB_i);             //Called at falling edges
        FU_QEntry* deQ();
};

class resStation;

#include "reserv_station.h"

class functionUnit
{
    public:
        FU_Q queue;
        FU_QEntry task;
        vector<resStation*> rs;
        functionUnit();
        void squash(int ROB_i);
        int next_vdd;
        pthread_t handle;
        virtual void FU_automat(){};
};

class intAdder : public functionUnit
{
    public:
        intAdder();
        ~intAdder();
        void FU_automat();
};

class flpAdder : public functionUnit
{
    private:
        FU_QEntry *pLatency_Q;
        void shift(FU_QEntry &in, FU_QEntry &out);
    public:
        flpAdder();
        ~flpAdder();
        void FU_automat();
};

class flpMtplr : public functionUnit
{
    private:
        FU_QEntry *pLatency_Q;
        void shift(FU_QEntry &in, FU_QEntry &out);
    public:
        flpMtplr();
        ~flpMtplr();
        void FU_automat();
};

class ldsdUnit: public functionUnit
{
    public:
        ldsdUnit();
        ~ldsdUnit();
        void FU_automat();
};

struct ROBEntry;

typedef struct bubbEntry
{
    int ROB_i;
    ROBEntry* ROB_E;
}bubbEntry;

class nopBublr
{
    private:
        bubbEntry bubble;
        bubbEntry shifter[3];
    public:
        pthread_t handle;
        int next_vdd;
        nopBublr();
        void generate_bubble(int ROB_i);
        void FU_automat();
};

void* intAdder_thread_container(void *arg);
void* flpAdder_thread_container(void *arg);
void* flpMlptr_thread_container(void *arg);
void* ldsdUnit_thread_container(void *arg);

void init_FUs();

#endif
