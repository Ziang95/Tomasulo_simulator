#ifndef MIPS_H
#define MIPS_H

#include <pthread.h>
#include <iostream>
#include <string>
#include <windows.h>

#define DEBUG_LEVEL 1

using namespace std;

typedef pthread_cond_t cond_t;
typedef pthread_mutex_t mutex_t;

class clk_tick{
    private:
        bool vdd;
    public:
        cond_t vdd_1; //Magic is here, pthread condition variable can make each thread be synced passively
        cond_t vdd_0;
        clk_tick();
        void set_vdd(bool val);
        bool get_vdd();
        bool oscillator(int freq); //This function oscillates [clk], at a frequency of [freq]
};

void msg_log(string msg, int lvl);
void err_log(string err); //Err message will always be displayed
void at_rising_edge(clk_tick &clk, mutex_t *lock);//Block the thread until [clk] ticks to 1
void at_falling_edge(clk_tick &clk, mutex_t *lock);//Block the thread until [clk] ticks to 0

#endif