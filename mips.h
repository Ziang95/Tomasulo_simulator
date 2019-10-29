#ifndef MIPS_H
#define MIPS_H

#include <pthread.h>
#include <iostream>
#include <string>
#include <windows.h>

typedef pthread_cond_t cond_t;
typedef pthread_mutex_t mutex_t;

#include "memory.h"

#define DEBUG_LEVEL 1
#define INT_REG_NUM 32
#define FP_REG_NUM 32
#define ROB_LEN 128
#define MEM_LEN 256

#define INTADDR_RS_NUM 2
#define INTADDR_EX_TIME 1
#define INTADDR_MEM_TIME 0

#define FPADDR_RS_NUM 3
#define FPADDR_EX_TIME 3
#define FPADDR_MEM_TIME 0

#define FPMTPLR_RS_NUM 2
#define FPMTPLR_EX_TIME 20
#define FPMTPLR_MEM_TIME 0

#define LD_STR_RS_NUM 3
#define LD_STR_EX_TIME 1
#define LD_STR_MEM_TIME 4

#define R0 0

using namespace std;

class clk_tick
{
    private:
        bool vdd;
    public:
        cond_t vdd_1; //Magic is here, pthread condition variable can make each thread be synced passively
        cond_t vdd_0; //Condition variable does not occupy system cycle while waiting
        clk_tick();
        bool get_vdd();
        bool oscillator(int freq); //This function oscillates [clk], at a frequency of [freq]
};

void msg_log(string msg, int lvl);      //When DEBUG_LEVEL >= lvl, general message will be displayed
void err_log(string err);               //Err message will always be displayed
void at_rising_edge(mutex_t *lock);     //Block the thread until [sys_clk] ticks to 1
void at_falling_edge(mutex_t *lock);    //Block the thread until [sys_clk] ticks to 0

#endif