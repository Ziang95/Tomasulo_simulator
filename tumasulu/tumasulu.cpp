// tumasulu.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Decleration of libraries and classes.
#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>
#include <fstream>
#include "RegFiles.h"
#include "InstBuffer.h"
#include "RAT.h"
#include "RsIU.h"
#include "RsFU.h"
#include "ROB.h"
#include "ExeTable.h"
#include "LdSdQueue.h"
#include "Import.h"
using namespace std;
//---------------------------------------------------------------------------------------------------------//
//---------------------------------******Global variables*******--------------------------------------------
//---------------------------------------------------------------------------------------------------------//
int cycles; 				// cycles count (starting from 1).
int pc; 					// program counter (used as a pointer for the instruction buffer).
int ETable_pc; 			// Execution Table pointer (used as a pointer for the instruction buffer).
int ETable_entries; 		// No. of Execution table entries (I guess it would be better if implemented ad linked list because you don't know how many entries will be).
int RegFiles_entries = 32; 	// No. of Register files entries (it is stated that they are fixed). 
int IQ_entries; 			// No. of Instruction Buffer entries (should be calculated from the input parser).
int Mem_entries = 64; 	// No. of Memory entries (it is stated that they are fixed).
int ROB_entries; 			// No. of Reorder buffer entries.
int LSQ_entries; 			// No. of Load Store Queue Rs entries.
int RSFA_entries; 		// No. of Floating point Adders Reservation Stations entries.
int RSFM_entries; 		// No. of Floating point Multiplier Reservation Stations entries.
int RSI_entries; 			// No. of Integer Reservation Stations entries.
 int FA_cycles;
 int FM_cycles;
 int I_cycles;
 int LS_cycles_ex;
 int LS_cycles_mem;
 int FA_funcUnits;
 int FM_funcUnits;
 int I_funcUnits;
 int LS_funcUnits;
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void intialize() {
	//creat inst. buffer and Register file
	Import input;
	input.ReadInput(); //read iput txt file and generating a new txt file with no extra spaces or commas
	input.SetVariables(); //getting needed info from txt file
	ROB_entries = input.ROB_entries;
	RSFM_entries = input.FP_Mul_rs;
	LSQ_entries = input.LdSd_rs;
	RSFA_entries = input.FP_adder_rs;
	RSI_entries = input.Int_adder_rs;
	FA_cycles = input.FP_adder_ex;
	FM_cycles = input.FP_Mul_ex;
	I_cycles = input.Int_adder_ex;
	LS_cycles_ex = input.LdSd_ex;
	LS_cycles_mem = input.LdSd_mem;
	FA_funcUnits = input.FP_adder_fu;
	FM_funcUnits = input.FP_Mul_fu;
	I_funcUnits = input.Int_adder_fu;
	LS_funcUnits = input.LdSd_fu;
	IQ_entries = (input.instructions.size()); //no of inst we have
	//cout << "no of inst " << IQ_entries << endl;
	// creating our instruction buffer
	InstBuffer *IQ = new InstBuffer[IQ_entries]; // pointer to instruction buffer
	for (int i = 0; i < IQ_entries; i++) {
		vector <string> temp = input.ReadInstruction(input.instructions[i]); //we know that temp size is always 4
		(*(IQ + i)).Opcode = temp[0];
		(*(IQ + i)).Dest = temp[1];
		(*(IQ + i)).src1 = temp[2];
		(*(IQ + i)).src2 = temp[3];
		//output test--------------------------------------------------------------------------------
		/*
		cout << "opcode " << i << (*(IQ + i)).Opcode << endl;
		cout << "Dest " << i << (*(IQ + i)).Dest << endl;
		cout << "src1 " << i << (*(IQ + i)).src1 << endl;
		cout << "src2 " << i << (*(IQ + i)).src2 << endl; */
		//------------------------------------------------------------------------------------------
	}
	//creating the 32 Register file we have (fixed)
		RegFiles *RF = new RegFiles[RegFiles_entries];
	// every register is now hardwired to it's number R0 is at RF[0] ans so on
	for (int i = 0; i < input.register_var.size(); i++) { // my format is R1=10 , F2=30.1 ... etc will get which reg
		string delimiter = "=";
		string token = input.register_var[i].substr(1, input.register_var[i].find(delimiter) - 1); //token1 is 1, 2 to R1 and R2 ...etc
		string value= input.register_var[i].substr(input.register_var[i].find(delimiter) +1,input.register_var[i].length());
		int Reg_number = input.convertStringToInt(token);
		if (input.register_var[i][0] == 'R') { //integer Register
			RF[Reg_number].intRegFile = input.convertStringToInt(value);
		}
		else { // float register
			RF[Reg_number].floatRegFile = input.convertStringToFloat(value);
		}
	} 

}
int main()
{
	intialize();
}

