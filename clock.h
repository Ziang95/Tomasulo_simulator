#ifndef CLOCK_H
#define CLOCK_H

#include <string>
#include <windows.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <iostream>

typedef pthread_cond_t cond_t;
typedef pthread_mutex_t mutex_t;

#include "mips.h"

using namespace std;

class clk_tick
{
    private:
        bool vdd;                   //Simulates the voltage drain-drain
        bool prog_started;          //Indicates whether the program has started
        bool prog_ended;
        int prog_cyc;               //Records the current cycle of program, 0 indicates the program hasn't started
    public:
        pthread_t handle;           //The handle of the thread running oscillator()
        cond_t vdd_1;               //Magic is here, pthread condition variable can make each thread be synced passively
        cond_t vdd_0;               //Condition variable does not occupy system cycle while waiting
        clk_tick();                 //Constructor
        bool get_vdd();             //Get the current clock vdd
        void reset_prog_cyc();      //Reset program cycle to 0
        int get_prog_cyc();         //Get the current program cycle.
        bool get_prog_ended();
        void end_prog();
        bool clk_automat(int freq);  //This function oscillates [clk], [freq] is the frequency of clock when no component is registered
};

void msg_log(string msg, int lvl);      //When DEBUG_LEVEL >= [lvl], general message [msg] will be displayed
void err_log(string err);               //Err message will always be displayed
void at_rising_edge(int &next_vdd);     //Block the thread until [sys_clk] ticks to 1, should only be called when [sys_clk]=0
void at_falling_edge(int &next_vdd);    //Block the thread until [sys_clk] ticks to 0, should only be called when [sys_clk]=1
void start_sys_clk();

#endif