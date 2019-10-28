// tumasulu.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Decleration of libraries and classes.
#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <cctype>
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
int inst_commit; // instruction in line to commit
Import input; //object of out input file
RegFiles* RF; // pointer to 32 register files
ExeTable* Extable; //pointer to extable
InstBuffer* IQ; // pointer to inst buffer
ROB* Rob; //pointer to ROB
RAT* Rat;// pointer to RAT

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void intialize() {
	//creat inst. buffer, Register file, exetable, ROB and intialize parameters needed
	//Import input;
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

	//creating ROB that has 128 entry(stated)---------------------------------------------------------------
	Rob = new ROB[ROB_entries];
    //-----------------------------------------------------------------------------------------------------
   // creating our instruction buffer

	IQ = new InstBuffer[IQ_entries]; // pointer to instruction buffer

	for (int i = 0; i < IQ_entries; i++) {
		vector <string> temp = input.ReadInstruction(input.instructions[i]); //we know that temp size is always 4
		(*(IQ + i)).Opcode = temp[0];
		(*(IQ + i)).Dest = temp[1];
		(*(Rob + i)).Dest = temp[1];
		(*(IQ + i)).src1 = temp[2];
		(*(IQ + i)).src2 = temp[3];
		//output test--------------------------------------------------------------------------------
	
		cout << "opcode " << i << (*(IQ + i)).Opcode << endl;
		cout << "Dest " << i << (*(IQ + i)).Dest << endl;
		cout<< "dest rob "<< i << (*(Rob +i)).Dest <<endl;
		cout << "src1 " << i << (*(IQ + i)).src1 << endl;
		cout << "src2 " << i << (*(IQ + i)).src2 << endl;  
		//------------------------------------------------------------------------------------------
	}
	//---------------------------------------------------------------------------------------------------------
	//creating the 32 Register file we have (fixed)

	RF = new RegFiles[RegFiles_entries];
	// every register is now hardwired to it's number R0 is at RF[0] ans so on

	for (int i = 0; i < input.register_var.size(); i++) { // my format is R1=10 , F2=30.1 ... etc will get which reg
		string delimiter = "=";
		string token = input.register_var[i].substr(1, input.register_var[i].find(delimiter) - 1); //token1 is 1, 2 to R1 and R2 ...etc
		string value = input.register_var[i].substr(input.register_var[i].find(delimiter) + 1, input.register_var[i].length());
		int Reg_number = input.convertStringToInt(token);
		if (input.register_var[i][0] == 'R') { //integer Register
			RF[Reg_number].intRegFile = input.convertStringToInt(value);
		}
		else { // float register
			RF[Reg_number].floatRegFile = input.convertStringToFloat(value);
		}
	}
	//cout << "R1 " << RF[1].intRegFile << " " << " R2 " << RF[2].intRegFile << " F2 " << RF[2].floatRegFile << endl;
	//----------------------------------------------------------------------------------------------------------------
	// creating Exetable and puting instructions in it 
	Extable = new ExeTable[IQ_entries];
	for (int i = 0; i < IQ_entries; i++) {
		Extable[i].Instruction = input.instructions[i];
		//cout << Extable[i].Instruction << endl;

	}
	

}

/*  void commit() {
	bool ready;
	int index;
	inst_commit = 0;
	string inst;
	inst = Extable[inst_commit].Instruction;
	transform(inst.begin(), inst.end(), inst.begin(), [](unsigned char c) { return std::tolower(c); });

	// special case is branches they don't commit get next inst

	if (inst.find("bne") || inst.find("beq") ){    // if next instruction is Branch then increment inst_commit
		inst_commit++; //instruction that should commit
	}

	for (int i = 0; i < ROB_entries; i++) {
		//if inst is fininshed calculating value and ready to commit in order
		ready = Rob[i].Finished && Rob[i].State.compare("commit") && (Rob[i].ETable_Entry == inst_commit);
		ready = ready && (Extable[Rob[i].ETable_Entry].WB < cycles);
		if (ready) {
			switch (Rob[i].Opcode)
			{
			case "ADD":
			case "ADDI":
			case "SUB":

				index = Integer.parseInt(Rob[i].Dest.replace("R", "")); // get index of register file
				string delimiter = "=";
				string token = input.register_var[i].substr(1, input.register_var[i].find(delimiter) - 1); //token1 is 1, 2 to R1 and R2
				RFiles[index].intRegFile = (int)ROB[i].Value; // update register file.

				// check whether to update the RAT or not.
				if (Rat[index].R.compare("ROB" + i))
					Rat[index].R = "R" + index;

				break;

			case "ADD.D":
			case "SUB.D":
			case "MULT.D":
			case "LD":

				index = Integer.parseInt(ROB[i].Dest.replace("F", "")); // get index of register file
				RFiles[index].floatRegFile = ROB[i].Value; // update register file.

				// check whether to update the RAT or not.
				if (RAT[index].F.equals("ROB" + i))
					RAT[index].F = "F" + index;

				break;

			case "SD": //make sure the value to be stored in memory is in ROB.Value

				if (Mem_busy)
					return;

				index = ROB[i].Addr; // get index of register file
				Mem[index].Value = ROB[i].Value; // update register file.

				Mem_busy = true;
				Cycle_Mem_busy = cycles;
				Mem_busy_sd = true;


				break;
			}

			ROB[i].Busy = false;
			ROB[i].State = "null";
			ETable[ROB[i].ETable_Entry].Commit = cycles;
			inst_commit++;

			break;
		}
	} 
}

		}

	}
	

} */
//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------****Main program****-------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
int main()
{
	intialize();
	//commit();
	//cout << IQ[0].Dest << endl;
	//cout << "R1 " << RF[1].intRegFile << " " << " R2 " << RF[2].intRegFile << " F2 " << RF[2].floatRegFile << endl;

}

