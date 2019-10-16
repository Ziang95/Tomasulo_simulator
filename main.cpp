#include <pthread.h>
#include <iostream>
#include <windows.h>
#include "test.h"
using namespace std;

#define DEBUG_LEVEL 2
#define ERR_LOG(err, lvl) if(DEBUG_LEVEL>=lvl)cerr<<__FILE__<<"-"<<__LINE__<<": "<<err<<endl
#define MSG_LOG(msg, lvl) if(DEBUG_LEVEL>=lvl)cout<<__FILE__<<"-"<<__LINE__<<": "<<msg<<endl

class clk_tick{
    // This class simulates the shifting VDD level in CPU
    private:
        bool internal_clk;
    public:
        void set_clk(bool set){
            internal_clk = set;
        }
        bool get_clk(){
            return internal_clk;
        }
};

bool oscillator(clk_tick &clk, int freq){
    //This function oscillates [clk], at a frequency of [freq]
    if (freq > 500){
        ERR_LOG("Frequency should be under 500", 1);
        return false;
    }
    while (true)
    {
        // cout<<clk.get_clk()<<endl;
        // Sleep(DWORD) is a windows.h API, accepts [32-bit] which is [long]
        clk.set_clk(1);
        Sleep(500/freq);
        clk.set_clk(0);
        Sleep(500/freq);
    }
    return true;
}

clk_tick sys_clk; // The primal VDD clock in CPU

void *sys_clk_thread(void *vargp){
    // Put oscillator into a thread
    oscillator(sys_clk, 250);
    return NULL;
}

void init_sys_clk(){
    // Starts the primal VDD clock and wait until it goes stable
    pthread_t clock;
    pthread_create(&clock, NULL, sys_clk_thread,NULL);
    MSG_LOG("Waiting for sys_clk to be stable...",1);
    int count = 0;
    while(count<500){
        if (sys_clk.get_clk())
            count++;
        Sleep(1);
    }
    MSG_LOG("sys_clk stablized",1);
}

int main(){
    init_sys_clk();
    cin.get();
}