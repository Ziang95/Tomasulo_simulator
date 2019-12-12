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
        bool vdd;                       //Simulates the voltage drain-drain
        bool prog_started;              //Indicates whether the program has started
        bool instr_ended;               //Indicates whether the instruction queue is finished
        bool mem_ended;                 //Indicates whether all memory operations have finished
        int prog_cyc;                   //Records the current cycle of program, 0 indicates the program hasn't started
    public:
        pthread_t handle;               //The handle of the thread running clk_automat()
        cond_t vdd_1;                   //Magic is here, pthread condition variable can make each thread be synced passively, Condition variable does not occupy system cycle while waiting
        cond_t vdd_0;                   //Magic is here, pthread condition variable can make each thread be synced passively, Condition variable does not occupy system cycle while waiting
        clk_tick();                     //Constructor
        bool get_vdd();                 //Get the current clock vdd
        void reset_prog_cyc();          //Reset program cycle to 0
        int get_prog_cyc();             //Get the current program cycle.
        bool is_instr_ended();          //Get [instr_ended]
        bool is_mem_ended();            //Get [mem_ended]
        void end_instr();               //Set [instr_ended] to true
        void end_mem();                 //Set [mem_ended] to true
        bool clk_automat(int freq);     //This function oscillates the clock, [freq] is the upbound of clock frequency, (when no component is registered)
};

void msg_log(string msg, int lvl);      //When debug_level >= [lvl], general message [msg] will be displayed
void err_log(string err);               //Err message will always be displayed
void at_rising_edge(int &next_vdd);     //Block the thread until [sys_clk] ticks to 1, should only be called when [sys_clk]=0
void at_falling_edge(int &next_vdd);    //Block the thread until [sys_clk] ticks to 0, should only be called when [sys_clk]=1
void start_sys_clk();                   //Starts the universal clock

#endif