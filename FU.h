
#ifndef FU_H
#define FU_H

#include <vector>
using namespace std;

#include "instruction.h"

typedef struct FU_QEntry
{
    bool done;
    opCode code;
    valType rtType;
    int dest;
    void *res;
    void *oprnd1;
    void *oprnd2;
    int offset;
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
        bool enQ(valType t, void *v, int s);
        int get_source();                      //Called at rising edges
        bool get_val(void *v);                 //Called at rising edges
        void CDB_automat();
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
        const FU_QEntry* enQ(opCode c, valType rt, int d, void *r, void *op1, void *op2, int offst);
        FU_QEntry* deQ();
};

class resStation;

#include "reserv_station.h"

class functionUnit
{
    private:
        int front = 0;
        int rear = 0;
    public:
        FU_Q queue;
        FU_QEntry task;
        vector<resStation*> rs;
        functionUnit();
        int next_vdd;
        pthread_t handle;
        virtual void FU_automate(){};
};

class intAdder : public functionUnit
{
    public:
        intAdder();
        ~intAdder();
        void FU_automate();
};

class flpAdder : public functionUnit
{
    public:
        flpAdder();
        ~flpAdder();
        void FU_automate();
};

class flpMtplr : public functionUnit
{
    public:
        flpMtplr();
        ~flpMtplr();
        void FU_automate();
};

class ldsdUnit: public functionUnit
{
    public:
        ldsdUnit();
        ~ldsdUnit();
        void FU_automate();
};

void* intAdder_thread_container(void *arg);
void* flpAdder_thread_container(void *arg);
void* flpMlptr_thread_container(void *arg);
void* ldsdUnit_thread_container(void *arg);

void init_FUs();

#endif
