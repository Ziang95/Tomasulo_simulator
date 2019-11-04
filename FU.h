#ifndef FU_H
#define FU_H

using namespace std;

typedef struct FU_QEntry
{
    bool done;
    int dest;
    void *res;
    void *oprnd1;
    void *oprnd2;
}FU_QEntry;

#include "mips.h"

class FU_Q
{
    private:
        FU_QEntry queue[Q_LEN];
        int front, rear;
        mutex_t deQ_lock;
    public:
        FU_Q();
        const FU_QEntry* enQ(int d, void *r, void *op1, void *op2);
        bool is_empty();
        FU_QEntry* deQ();
};

class functionUnit
{
    private:
        int front, rear;
    public:
        FU_QEntry QEntry;
        functionUnit();
        int next_vdd;
        pthread_t handle;
        virtual void FU_automate(){};
};

class intAdder : public functionUnit
{
    public:
        void FU_automate();
};

class flpAdder : public functionUnit
{
    public:
        void FU_automate();
};

class flpMtplr : public functionUnit
{
    public:
        void FU_automate();
};

#endif
