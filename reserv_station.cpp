#include "reserv_station.h"

reservCDB::reservCDB()
{
    front = rear = 0;
}

bool reservCDB::enQ(valType t, void *v, int s)
{
    if ((rear+1)%128 == front)
    {
        err_log("The reserv_CDB queue is full");
        return false;
    }
    int index = rear;
    rear = (++rear)%128;
    queue[index].type = t;
    if (t == INTGR)
        queue[index].value.i = *(int*)v;
    else
        queue[index].value.f = *(float*)v;
    queue[index].source = s;
    return true;
}

int reservCDB::get_source()
{
    if (front == rear)
        return -1;
    return queue[front].source;
}

bool reservCDB::get_val(void *v)
{
    if (queue[front].type == INTGR)
        *(int*)v = queue[front].value.i;
    else
        *(float*)v = queue[front].value.f;
    return true;
}