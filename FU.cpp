#include "FU.h"

extern config *CPU_cfg;
extern reservCDB rCDB;
extern FU_Q intAdder_Q;
extern FU_Q flpAdder_Q;
extern FU_Q flpMtplr_Q;

FU_Q::FU_Q()
{
    front = rear = 0;
    deQ_lock = PTHREAD_MUTEX_INITIALIZER;
}

const FU_QEntry* FU_Q::enQ(int d, void *r, void *op1, void *op2)
{
    if ((rear+1)%Q_LEN == front)
        throw -1;
    int index = rear;
    rear = (++rear)%Q_LEN;
    queue[index].dest = d;
    queue[index].res = r;
    queue[index].oprnd1 = op1;
    queue[index].oprnd2 = op2;
    queue[index].done = false;
    return &queue[index];
}

FU_QEntry *FU_Q::deQ()
{
    pthread_mutex_lock(&deQ_lock);
    if (front == rear)
    {
        pthread_mutex_unlock(&deQ_lock);
        return nullptr;
    }
    FU_QEntry* ret = &queue[front];
    front = (++front)%Q_LEN;
    pthread_mutex_unlock(&deQ_lock);
    return ret;
}

functionUnit::functionUnit()
{
    next_vdd = 1;
    front = rear = 0;
    QEntry.done = true;
}

void intAdder::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (!QEntry.done)
        {
            for (int i = 0; ;i++)
            {
                msg_log("Calculating int "+to_string(*(int*)QEntry.oprnd1)+" + "+to_string(*(int*)QEntry.oprnd2), 3);
                at_falling_edge(&lock, next_vdd);
                if (i == CPU_cfg->int_add->exe_time - 1)
                {
                    *(int*)QEntry.res = *(int*)QEntry.oprnd1 + *(int*)QEntry.oprnd2;
                    rCDB.enQ(INTGR, QEntry.res, QEntry.dest);
                    QEntry.done = true;
                    break;
                }
                at_rising_edge(&lock, next_vdd);
            }
        }
        else
            at_falling_edge(&lock, next_vdd);
        auto newTask = intAdder_Q.deQ();
        if (newTask != nullptr)
        {
            QEntry.dest = newTask->dest;
            QEntry.oprnd1 = newTask->oprnd1;
            QEntry.oprnd2 = newTask->oprnd2;
            QEntry.res = newTask->res;
            QEntry.done = false;
        }
    }
}

void flpAdder::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (!QEntry.done)
        {
            for (int i = 0; ;i++)
            {
                msg_log("Calculating flp "+to_string(*(float*)QEntry.oprnd1)+" + "+to_string(*(float*)QEntry.oprnd2), 3);
                at_falling_edge(&lock, next_vdd);
                if (i == CPU_cfg->fp_add->exe_time - 1)
                {
                    *(float*)QEntry.res = *(float*)QEntry.oprnd1 + *(float*)QEntry.oprnd2;
                    rCDB.enQ(FLTP, QEntry.res, QEntry.dest);
                    QEntry.done = true;
                    break;
                }
                at_rising_edge(&lock, next_vdd);
            }
        }
        else
            at_falling_edge(&lock, next_vdd);
        auto newTask = flpAdder_Q.deQ();
        if (newTask != nullptr)
        {
            QEntry.dest = newTask->dest;
            QEntry.oprnd1 = newTask->oprnd1;
            QEntry.oprnd2 = newTask->oprnd2;
            QEntry.res = newTask->res;
            QEntry.done = false;
        }
    }
}

void flpMtplr::FU_automate()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while (true)
    {
        at_rising_edge(&lock, next_vdd);
        if (!QEntry.done)
        {
            for (int i = 0; ;i++)
            {
                msg_log("Calculating flp"+to_string(*(float*)QEntry.oprnd1)+" X "+to_string(*(float*)QEntry.oprnd2), 3);
                at_falling_edge(&lock, next_vdd);
                if (i == CPU_cfg->fp_mul->exe_time - 1)
                {
                    *(float*)QEntry.res = *(float*)QEntry.oprnd1 * *(float*)QEntry.oprnd2;
                    rCDB.enQ(FLTP, QEntry.res, QEntry.dest);
                    QEntry.done = true;
                    break;
                }
                at_rising_edge(&lock, next_vdd);
            }
        }
        else
            at_falling_edge(&lock, next_vdd);
        auto newTask = flpMtplr_Q.deQ();
        if (newTask != nullptr)
        {
            QEntry.dest = newTask->dest;
            QEntry.oprnd1 = newTask->oprnd1;
            QEntry.oprnd2 = newTask->oprnd2;
            QEntry.res = newTask->res;
            QEntry.done = false;
        }
    }
}