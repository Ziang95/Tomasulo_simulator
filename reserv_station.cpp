#include "reserv_station.h"
#include "mips.h"

extern vector<int*> clk_wait_list;
extern reservCDB rCDB;

reservCDB::reservCDB()
{
    next_vdd = 1;
    front = rear = 0;
    bus.source = -1;
}

bool reservCDB::enQ(valType t, void *v, int s)
{
    if ((rear+1)%Q_LEN == front)
    {
        err_log("The reserv_CDB queue is full");
        return false;
    }
    int index = rear;
    rear = (++rear)%Q_LEN;
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
    return bus.source;
}

bool reservCDB::get_val(void *v)
{
    if (bus.type == INTGR)
        *(int*)v = bus.value.i;
    else
        *(float*)v = bus.value.f;
    return true;
}

void reservCDB::CDB_automat()
{
    mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    while(true)
    {
        at_rising_edge(&lock, next_vdd);
        at_falling_edge(&lock, next_vdd);
        if (front != rear)
        {
            bus.source = queue[front].source;
            bus.type = queue[front].type;
            bus.value = queue[front].value;
            front = (++front)%Q_LEN;
        }
    }
}

void init_reserv_CDB()
{
    rCDB.next_vdd = 1;
    clk_wait_list.push_back(&rCDB.next_vdd);
    pthread_create(&rCDB.handle, NULL, [](void *arg)->void*{rCDB.CDB_automat(); return NULL;}, NULL);
}