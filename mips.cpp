#include "mips.h"
#include "config.h"
#include "InstBuffer.h"
#include "RegFiles.h"
#include "ROB.h"

using namespace std;

config Config;
clk_tick sys_clk = clk_tick(); //System clock
memory main_mem = memory(MEM_LEN); //Main memory 
mutex_t cout_lock = PTHREAD_MUTEX_INITIALIZER;
mutex_t cerr_lock = PTHREAD_MUTEX_INITIALIZER;
int IQ_entries; 			// No. of Instruction Buffer entries (should be calculated from the input parser).
int ROB_Entries;
int RegFiles_entries = 32; 	// No. of Register files entries (it is stated that they are fixed). 
RegFiles* RF; // pointer to 32 register files
InstBuffer* IQ; // pointer to inst buffer
ROB* Rob; //pointer to ROB


clk_tick::clk_tick()
{
    vdd_0 = vdd_1 = PTHREAD_COND_INITIALIZER;
    vdd = 0;
    prog_started = false;
    prog_cyc = 0;
}

bool clk_tick::get_vdd()
{
    return vdd;
}

int clk_tick::get_prog_cyc()
{
    return prog_cyc;
}

void clk_tick::start_prog()
{
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    at_falling_edge(&clk);
    pthread_cond_broadcast(&start);
    prog_started = true;
}

bool clk_tick::oscillator(int freq)
{
    if (freq > 500)
    {
        err_log("Frequency should be under 500");
        return false;
    }
    while (true)
    {
        if (prog_started)
            prog_cyc++;
        vdd = 1;
        pthread_cond_broadcast(&vdd_1);
        Sleep(500/freq);
        vdd = 0;
        pthread_cond_broadcast(&vdd_0);
        Sleep(500/freq);
    }
}

void msg_log(string msg, int lvl)
{
    pthread_mutex_lock(&cout_lock);
    if (DEBUG_LEVEL>=lvl)
        cout<<"[Pro_cyc="<<sys_clk.get_prog_cyc()<<", vdd="<<sys_clk.get_vdd()<<"] "<<msg<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void err_log(string err)
{
    pthread_mutex_lock(&cout_lock);
    cout<<"[Pro_cyc="<<sys_clk.get_prog_cyc()<<", vdd="<<sys_clk.get_vdd()<<"] "<<err<<endl;
    pthread_mutex_unlock(&cout_lock);
}

void at_rising_edge(mutex_t *lock)
{
    pthread_mutex_lock(lock);
    while(sys_clk.get_vdd() == 0)
        pthread_cond_wait(&(sys_clk.vdd_1), lock);
    pthread_mutex_unlock(lock);
}

void at_falling_edge(mutex_t *lock)
{
    pthread_mutex_lock(lock);
    while(sys_clk.get_vdd() == 1)
        pthread_cond_wait(&(sys_clk.vdd_0), lock);
    pthread_mutex_unlock(lock);
}

void init_sys_clk() // Starts the primal VDD clock and wait until it goes stable (covered 500 cycles)
{
    pthread_create(&(sys_clk.handle), NULL, [](void *arg)->void*{sys_clk.oscillator(100);return NULL;},NULL);
    msg_log("Waiting for sys_clk to be stable...", 0);
    int count = 0;
    mutex_t clk = PTHREAD_MUTEX_INITIALIZER;
    while(count<100)
    {
        at_rising_edge(&clk);
        count++;
        at_falling_edge(&clk);
    }
    msg_log("sys_clk stablized", 0);
}
void initialize(){

	//creat inst. buffer, Register file, ROB and intialize parameters needed

Config.ReadInput();
Config.SetVariables();
    ROB_Entries = Config.getROB_LEN();
	IQ_entries = (Config.instructions.size()); //no of inst we have

	//creating ROB that has 128 entry(stated)---------------------------------------------------------------
	Rob = new ROB[ROB_Entries];
    //-----------------------------------------------------------------------------------------------------
   // creating our instruction buffer

	IQ = new InstBuffer[IQ_entries]; 

	for (int i = 0; i < IQ_entries; i++) {
		vector <string> temp = Config.ReadInstruction(Config.instructions[i]); //we know that temp size is always 4
		(*(IQ + i)).Opcode = temp[0];
		(*(IQ + i)).Dest = temp[1];
		(*(Rob + i)).Dest = temp[1];
		(*(IQ + i)).src1 = temp[2];
		(*(IQ + i)).src2 = temp[3];
}
//creating the 32 Register file we have (fixed)

	RF = new RegFiles[RegFiles_entries];
	// every register is now hardwired to it's number R0 is at RF[0] ans so on

	for (int i = 0; i < Config.register_var.size(); i++) { // my format is R1=10 , F2=30.1 ... etc will get which reg
		string delimiter = "=";
		string token = Config.register_var[i].substr(1, Config.register_var[i].find(delimiter) - 1); //token1 is 1, 2 to R1 and R2 ...etc
		string value = Config.register_var[i].substr(Config.register_var[i].find(delimiter) + 1, Config.register_var[i].length());
		int Reg_number = Config.convertStringToInt(token);
		if (Config.register_var[i][0] == 'R') { //integer Register
			RF[Reg_number].intRegFile = Config.convertStringToInt(value);
		}
		else { // float register
			RF[Reg_number].floatRegFile = Config.convertStringToFloat(value);
		}
	}
	//----------------------------------------------------------------------------------------------------------------
	
	
}

int main()
{
    init_sys_clk();
    init_main_mem();
    sys_clk.start_prog();
    int i = 110, li;
    float f = 10.1, lf;
    main_mem.enQ(STORE, INTGR, 1, &i);
    main_mem.enQ(STORE, FLTP, 2, &f);
    main_mem.enQ(LOAD, INTGR, 1, &li);
    main_mem.enQ(LOAD, FLTP, 2, &lf);
    cin.get();
    initialize();
}