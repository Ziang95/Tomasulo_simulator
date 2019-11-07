// tumasulu.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Decleration of libraries and classes.
#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>
#include <deque>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <cctype>
#include "RegFiles.h"
#include "InstBuffer.h"
#include "RAT.h"
#include "RsIU.h"
#include "RsFU.h"
#include "Memory.h"
#include "ROB.h"
#include "ExeTable.h"
#include "LdSdQueue.h"
#include "Import.h"
#include "queue.h"
#include "Output.h"

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
bool IU_busy;			// if IU is busy 
bool Mem_busy;			// if Mem is busy 
bool Mem_busy_sd;		// if Mem is busy due to store 
int Cycle_Mem_busy;		// the cycle the meory was busy in.
bool CDB_busy;			// if Mem is busy 
bool Stall;			    // whether to stall or not.
int inst_count;			// no of the instruction for commiting purposes.
int inst_commit;		// the instruction that should commit.
int LSQ_count;			// LSQ count.
bool dont_fetch;		// the instruction that should commit.

//------------------------------------------------------------------------------------------------------------------
//-------- Project Arch---------------------------------------------------------------------------------------------

Import input;                                                           // object of out input file
RegFiles* RF = new RegFiles[RegFiles_entries];                          // pointer to 32 register files
ExeTable* Extable;                                                      // pointer to extable
InstBuffer* IQ;                                                         // pointer to inst buffer
ROB* Rob;                                                               // pointer to ROB
RAT* Rat;                                                               // pointer to RAT
Memory* Mem = new Memory[Mem_entries];                                  // pointer to memory
queue<LdSdQueue> LSQ; 					                                // Global definition of LSQ.
RsFU* RSFA; 												            // Global definition of RSFA.
RsFU* RSFM; 											            	// Global definition of RSFM.
RsIU* RSI; 												                // Global definition of RSI.


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////
//////////////// Functions Implementations ////////////////
//////////////////////////////////////////////////////////

void Arch_unit() {
	cycles = 1; 	// start from cycle 1.
	pc = 0; 		// first instruction.
	ETable_pc = 0; 	// ExecTable pointer.
	Mem_busy = false;
	Mem_busy_sd = false;
	IU_busy = false;
	CDB_busy = false;
	Stall = false;
	inst_count = 0;
	inst_commit = 0;
	dont_fetch = false;
	LSQ_count = 0;
	//-------------------------------------------------------------------
		// creating Exetable and puting instructions in it 
	Extable = new ExeTable[ETable_entries];
	for (int i = 0; i < ETable_entries; i++) {
		 // initialize each entry in the Execution Table.
		Extable[i].Mem = 0;
	}
	//Extable = new ExeTable[IQ_entries];
	for (int i = 0; i < IQ_entries; i++) {
		Extable[i].Instruction = input.instructions[i];
		//cout << Extable[i].Instruction << endl;

	}
	

	//------------------------------------------------------------------
		/// creating intial RAT which will have R1, R2,.. etc as intial value.
	Rat = new RAT[RegFiles_entries];

	for (int i = 0; i < RegFiles_entries; i++)
	{
		Rat[i].R = "R" + to_string(i);
		Rat[i].F = "F" + to_string(i);
		//cout << Rat[i].R << endl;
		//cout << Rat[i].F << endl;
	}
	//-----------------------------------------------------------------------
	//intializing memory

	for (int i = 0; i < Mem_entries; i++) {
		Mem[i].Value = 0.0;
	}

	for (int i = 0; i < input.memory_var.size(); i++)
	{
		// my format is Mem[4]=1 , Mem[12]=3.4 ... etc
		string delimiter = "]";
		string token = input.memory_var[i].substr(4, input.memory_var[i].find(delimiter) - 4); //token1 is 1, 2,......,etc
		string value = input.memory_var[i].substr(input.memory_var[i].find(delimiter) + 2, input.memory_var[i].length());
		int Mem_entry = input.convertStringToInt(token);
		Mem[Mem_entry].Value = input.convertStringToFloat(value);

	}

	for (int i = 0; i < Mem_entries; i++)
	{

		if (Mem[i].Value == NULL) // because some will initialized in the input parser
		{
			Mem[i].Value = 0.0;
			Mem[i].ischanged = false;
		}
	}

	//--------------------------------------------------------------------------------------------------------
	RSFM = new RsFU[RSFM_entries]; 		// initialize each entry in the RSFM.
	for (int i = 0; i < RSFM_entries; i++)
	{
		RSFM[i].Busy = false;
		RSFM[i].Qj = "NULL";
		RSFM[i].Qk = "NULL";

	}

	RSFA = new RsFU[RSFA_entries]; 		// initialize each entry in the RSFA.

	for (int i = 0; i < RSFA_entries; i++)
	{
		RSFA[i].Busy = false;
		RSFA[i].Qj = "NULL";
		RSFA[i].Qk = "NULL";
	}
	RSI = new RsIU[RSI_entries]; 		// initialize each entry in the RSI.

	for (int i = 0; i < RSI_entries; i++)
	{
		RSI[i].Busy = false;
		RSI[i].Qj = "NULL";
		RSI[i].Qk = "NULL";
	}



}
//------------------------------------------------------------------------------------------------------------------------

void Fetch()
{
	string temp;
	int index;
	LdSdQueue* LSQ_temp = new LdSdQueue();
	bool f;


L1:	if (pc >= IQ_entries)
{
	dont_fetch = true;
	Stall = true;
	return;
}
string Opcode = IQ[pc].Opcode;
for_each(Opcode.begin(), Opcode.end(), [](char& c) {
	c = ::toupper(c);
	});
//cout << Opcode << endl;
if (Opcode == "LD") {
	if (LSQ_count < LSQ_entries) {
		// we have a space in Load store Queue (you can check from any field).
		for (int j = 0; j < ROB_entries; j++) {
			if (!Rob[j].Busy) {
				//find a free ROB entry
				LSQ_temp->Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + "(" + IQ[pc].Src2 + ")"; // store whole instruction for debugging purposes.
				LSQ_temp->Opcode = IQ[pc].Opcode; // may be removed later if of no use.
				LSQ_temp->ETable_Entry = ETable_pc;
				LSQ_temp->ROB_entry = j;
				LSQ_temp->State = "NULL";
				Rob[j].Busy = true;
				Rob[j].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + "(" + IQ[pc].Src2 + ")"; // store whole instruction for debugging purposes.
				Rob[j].Dest = IQ[pc].Dest; // store the destination in ROB entry.
				Rob[j].Opcode = IQ[pc].Opcode;
				Rob[j].ETable_Entry = ETable_pc;
				Rob[j].State = "NULL";
				//cout << LSQ_temp->Inst << endl;
				//cout << Rob[j].Inst << endl;
				// resolving Src1
				//cout << IQ[pc].Src2 << endl;
				if (IQ[pc].Src2.find("R") != string::npos) {
					temp = IQ[pc].Src2.substr(1, IQ[pc].Src2.find('R') - 1); // remove R to convert it to integer.
					index = input.convertStringToInt(temp);
					//cout << index << endl;
					if (index < RegFiles_entries) {
						//correct input range.
						//Rat[1].R = "ROB3";
						//Rob[3].Value = 100.6;
						//Rob[3].State = "Write_Back";
						if (Rat[index].R.find("ROB") != string::npos) // Src2 is in ROB
						{
							temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
							index = input.convertStringToInt(temp);
							//cout << index << endl;
							// check if this entry is in the write back or commit stage.
							if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
							{
								int conv = (int)Rob[index].Value;
								LSQ_temp->Reg = to_string(conv); // copy from ROB to LSQ
								//cout << LSQ_temp->Reg << endl;
								try
								{
									input.convertStringToInt(LSQ_temp->Reg);
									LSQ_temp->State = "Ready";
								}
								catch (exception e) // invalid format
								{

								}
								//cout <<"LD1"<< LSQ_temp->State << endl;

							}
							else
							{ // instruction is not finished yet, store ROB entry in LSQ.Reg.
								LSQ_temp->Reg = "ROB" + to_string(index);
								//cout << LSQ_temp->Reg <<"Rob plus index"<< endl;

							}
						}
						else if (Rat[index].R.find("R") != string::npos) // Src2 is in ARF
						{
							temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
							index = input.convertStringToInt(temp);
							int conv = RF[index].intRegFile;
							LSQ_temp->Reg = to_string(conv); // copy from ARF to LSQ
							//cout << LSQ_temp->Reg << "Found in arf" << endl;
							try
							{
								input.convertStringToInt(LSQ_temp->Reg);
								LSQ_temp->State = "Ready";
							}
							catch (exception e) // invalid format
							{
							}
							//cout <<"LD2"<< LSQ_temp->State << endl;

						}

					}
					else // Src1 input range is exceeded.
					{
						cout << "Src1 exceeded range of register files." << endl;
						exit(1); // exit code.
					}
				}
				else // wrong Src2 format
				{
					cout << "Src1 field is wrong, should be in the format of 'Rx'" << endl;
					exit(1); // exit code.
				}
				// resolving Src2, immediate value, just copy to LSQ.Offset.

				try
				{
					LSQ_temp->Offset = input.convertStringToInt(IQ[pc].Src1);
				}
				catch (exception e) // invalid format
				{
					cout << "Src2 field for Ld/St operations should be integer" << endl;
					exit(1); // exit code.
				}
				//cout << LSQ_temp->Offset << endl;
				// store destination in LSQ.Dest if it is a load

				if (IQ[pc].Dest.find("F") != string::npos)
				{
					temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('F') - 1);// remove F to convert it to integer.
					index = input.convertStringToInt(temp);
					Rat[index].F = "ROB" + to_string(j); 	// store the ROB destination.
					LSQ_temp->Dest = "ROB" + to_string(j); 	// store the ROB destination.
				}
				else // wrong Dest format
				{
					cout << "Dest field is wrong, should be in the format of 'Rx'" << endl;
					exit(1); // exit code.
				}

				// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
				pc++;
				Extable[ETable_pc].Instruction = LSQ_temp->Inst;
				Extable[ETable_pc].Issue = cycles;
				ETable_pc++;
				LSQ.push(*LSQ_temp); // add the entry to the lSQ.
				LSQ_count++;
				break; // break the loop found an empty ROB
			}
			//break;
		}
		
	}

	//goto L1;
}
else if (Opcode == "SD") {
	if (LSQ_count < LSQ_entries) // we have a space in Load store Queue (you can check from any field).
	{
		for (int j = 0; j < ROB_entries; j++) //searching for an empty ROB space
		{
			if (!Rob[j].Busy) {

				//find a free ROB entry
				LSQ_temp->Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + "(" + IQ[pc].Src2 + ")"; // store whole instruction for debugging purposes.
				LSQ_temp->Opcode = IQ[pc].Opcode; // may be removed later if of no use.
				LSQ_temp->ETable_Entry = ETable_pc;
				LSQ_temp->ROB_entry = j;
				LSQ_temp->State = "NULL";
				Rob[j].Busy = true;
				Rob[j].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + "(" + IQ[pc].Src2 + ")"; // store whole instruction for debugging purposes.
				Rob[j].Dest = IQ[pc].Dest; // store the destination in ROB entry.
				Rob[j].Opcode = IQ[pc].Opcode;
				Rob[j].ETable_Entry = ETable_pc;
				Rob[j].State = "NULL";
				//cout << LSQ_temp->Inst << endl;
				//cout << Rob[j].Inst << endl;
				// resolving Src1

				//cout << IQ[pc].Src2 << endl;
				if (IQ[pc].Src2.find("R") != string::npos) {
					temp = IQ[pc].Src2.substr(1, IQ[pc].Src2.find('R') - 1); // remove R to convert it to integer.
					index = input.convertStringToInt(temp);
					//cout << index << endl;
					if (index < RegFiles_entries) {
						//correct input range.
						//Rat[2].R = "ROB4";
						//Rob[4].Value = 101.9;
						//Rob[4].State = "Write_Back";
						if (Rat[index].R.find("ROB") != string::npos) // Src2 is in ROB
						{
							temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
							index = input.convertStringToInt(temp);
							//cout << index << endl;
							// check if this entry is in the write back or commit stage.
							if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
							{
								int conv = (int)Rob[index].Value;
								LSQ_temp->Reg = to_string(conv); // copy from ROB to LSQ
								//cout << LSQ_temp->Reg << endl;
								try
								{
									input.convertStringToInt(LSQ_temp->Reg);
									LSQ_temp->State = "Ready";
								}
								catch (exception e) // invalid format
								{
								}
								//cout << "SD1"<<LSQ_temp->State << endl;
							}
							else
							{ // instruction is not finished yet, store ROB entry in LSQ.Reg.
								LSQ_temp->Reg = "ROB" + to_string(index);
								//cout << LSQ_temp->Reg << endl;
							}
						}
						else if (Rat[index].R.find("R") != string::npos) // Src2 is in ARF
						{
							temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
							index = input.convertStringToInt(temp);
							int conv = RF[index].intRegFile;
							LSQ_temp->Reg = to_string(conv); // copy from ARF to LSQ
							try
							{
								input.convertStringToInt(LSQ_temp->Reg);
								LSQ_temp->State = "Ready";
							}
							catch (exception e) // invalid format
							{
							}
							//cout << "SD2" << LSQ_temp->State << endl;

						}

					}
					else // Src1 input range is exceeded.
					{
						cout << "Src1 exceeded range of register files." << endl;
						exit(1); // exit code.
					}
				}
				else // wrong Src2 format
				{
					cout << "Src1 field is wrong, should be in the format of 'Rx'" << endl;
					exit(1); // exit code.
				}
				// resolving Src2, immediate value, just copy to LSQ.Offset.

				try
				{
					LSQ_temp->Offset = input.convertStringToInt(IQ[pc].Src1);
				}
				catch (exception e) // invalid format
				{
					cout << "Src2 field for Ld/St operations should be integer" << endl;
					exit(1); // exit code.
				}
				//cout << LSQ_temp->Offset << endl;
				// store destination in LSQ.Dest if it is a load

				if (IQ[pc].Dest.find("F") != string::npos)
				{
					temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('F') - 1);// remove F to convert it to integer.
					index = input.convertStringToInt(temp);
					Rat[index].F = "ROB" + to_string(j); 	// store the ROB destination.
					LSQ_temp->Dest = "ROB" + to_string(j); 	// store the ROB destination.
				}
				else // wrong Dest format
				{
					cout << "Dest field is wrong, should be in the format of 'Rx'" << endl;
					exit(1); // exit code.
				}

				// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
				pc++;
				Extable[ETable_pc].Instruction = LSQ_temp->Inst;
				Extable[ETable_pc].Issue = cycles;
				ETable_pc++;
				LSQ.push(*LSQ_temp); // add the entry to the lSQ.
				LSQ_count++;
				break; // break the loop found an empty ROB

			}
		//	break;
		}

	}
	//goto L1;
}


else if (Opcode == "BEQ" || Opcode == "BNE") {
	//  offset is Dest field, Rs and Rt is Src1 and Src2.
	// 3 is dest is Src2 , R2 is src1 is Dest , R3 is Scr2 is Src1
	for (int i = 0; i < RSI_entries; i++) {
		if (!RSI[i].Busy)
		{                //find an empty reservation station
			Stall = true; // stop fetching till you figure out the destination address.
			RSI[i].Busy = true;
			RSI[i].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
			RSI[i].Opcode = IQ[pc].Opcode; // may be removed later if of no use.
			RSI[i].ETable_Entry = ETable_pc;
			//cout << RSI[i].Inst << endl;
			// resolving dest

			if (IQ[pc].Dest.find("R") != string::npos)
			{
				temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('R') - 1); // remove R to convert it to integer.
				index = input.convertStringToInt(temp);
				//cout << "dest index "<<index << endl;
				//Rat[2].R = "ROB3";
				//Rob[3].State = "Write_Back";
				//Rob[3].Value = 55.0;
				if (index < RegFiles_entries) // correct input range.
				{
					if (Rat[index].R.find("ROB") != string::npos) // dest is in ROB
					{
						temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << "3 is " << index << endl;
						// check if this entry is in the write back or commit stage.
						if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
						{
							RSI[i].Vj = (int)Rob[index].Value; // copy from ROB to RSI
							//cout << "Vj " << RSI[i].Vj << endl;
							//cout << "it's empty Qj "<<RSI[i].Qj << endl;

						}
						else // instruction is not finished yet, store ROB entry in Qj.
						{
							RSI[i].Qj = "ROB" + to_string(index);
							//cout << "Qj " <<RSI[i].Qj << endl;
						}
					}
					else if (Rat[index].R.find("R") != string::npos) // dest is in ARF
					{
						temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						RSI[i].Vj = RF[index].intRegFile; // copy from ARF to RSI
						//cout << "Vj in ARF" << RSI[i].Vj << endl;
					}
				}
				else // dest input range is exceeded.
				{
					cout << "Dest exceeded range of register files." << endl;
					exit(1); // exit code.
				}
			}
			else // wrong dest format
			{
				cout << "Dest field is wrong, should be in the format of 'Rx'" << endl;
				exit(1); // exit code.
			}
			// resolving Src1

			if (IQ[pc].Src1.find("R") != string::npos)
			{
				temp = IQ[pc].Src1.substr(1,IQ[pc].Src1.find('R') - 1); // remove R to convert it to integer.
				index = input.convertStringToInt(temp);
				//cout << "3 is" << index << endl;
				//Rat[3].R = "ROB5";
				//Rob[5].State = "Write_Back";
				//Rob[5].Value = 509.45;
				if (index < RegFiles_entries) // correct input range.
				{
					if (Rat[index].R.find("ROB") != string::npos) // src1 is in ROB
					{
						temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << "5 is " << index << endl;
						// check if this entry is in the write back or commit stage.
						if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
						{
							RSI[i].Vk = (int)Rob[index].Value; // copy from ROB to RSI
							//cout<<"vk "<<RSI[i].Vk << endl;
							if (RSI[i].Qj == "NULL") {
								RSI[i].State = "Ready";
							}
							//cout << RSI[i].State << endl;
						}
						else // instruction is not finished yet, store ROB entry in Qk.
						{
							RSI[i].Qk = "ROB" + to_string(index);
							//cout << RSI[i].Qk << endl;
						}
					}
					else if (Rat[index].R.find("R") != string::npos) // Src1 is in ARF
					{
						temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						RSI[i].Vk = RF[index].intRegFile; // copy from ARF to RSI
						//cout << RSI[i].Vk << endl;
						if (RSI[i].Qj == "NULL") {
							RSI[i].State = "Ready";
						}
						//cout << RSI[i].State << endl;
					}
				}
				else // Src1 input range is exceeded.
				{
					cout << "Src1 exceeded range of register files." << endl;;
					exit(1); // exit code.
				}
			}
			else // wrong Src1 format
			{
				cout << "Src1 field is wrong, should be in the format of 'Rx'" << endl;
				exit(1); // exit code.
			}
			// store destination offset in Addr field of RSI.

			try
			{
				RSI[i].Addr = input.convertStringToInt(IQ[pc].Src2);
				//cout << RSI[i].Addr << endl;
			}
			catch (exception e) // invalid format
			{
				cout << "Scr2 field for Branch operations should be integer" << endl;
				exit(1); // exit code.
			}
			// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
						// we should stall other operations here till the branch is resolved.
			pc++;
			Extable[ETable_pc].Instruction = RSI[i].Inst;
			Extable[ETable_pc].Issue = cycles;
			//cout<<Extable[ETable_pc].Instruction << endl;
			//cout << Extable[ETable_pc].Issue << endl;
			ETable_pc++;
			break;
		}
		//break;
	}
	
//	goto L1;
}
else if (Opcode == "ADD" || Opcode == "SUB") {

	for (int i = 0; i < RSI_entries; i++) {
		if (!RSI[i].Busy) // find an empty reservation station.
		{
			for (int j = 0; j < ROB_entries; j++) {
				if (!Rob[j].Busy) // find an empty ROB entry
				{
					RSI[i].Busy = true;
					RSI[i].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					RSI[i].Opcode = IQ[pc].Opcode; // may be removed later if of no use.
					RSI[i].ETable_Entry = ETable_pc;
					//cout << RSI[i].Inst << endl;
					Rob[j].Busy = true;
					Rob[j].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					Rob[j].Dest = IQ[pc].Dest; // store the destination in ROB entry.
					Rob[j].Opcode = IQ[pc].Opcode;
					Rob[j].ETable_Entry = ETable_pc;
					Rob[j].State = "NULL";
					//cout << Rob[j].Inst << endl;
					//cout << Rob[j].State << Rob[j].Opcode << endl;
					// resolving Src1

					if (IQ[pc].Src1.find("R") != string::npos)
					{
						temp = IQ[pc].Src1.substr(1, IQ[pc].Src1.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;
						//Rat[2].R = "ROB5";
						//Rob[5].State = "Write_Back";
						//Rob[5].Value = 57.97;
						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].R.find("ROB") != string::npos) // Src1 is in ROB
							{
								temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
								{
									RSI[i].Vj = (int)Rob[index].Value; // copy from ROB to RSI
									//cout << RSI[i].Vj << endl;
								}
								else // instruction is not finished yet, store ROB entry in Qj.
								{
									RSI[i].Qj = "ROB" + to_string(index);
									//cout << RSI[i].Qj << endl;
									//cout << RSI[i].Qk << endl;
								}
							}
							else if (Rat[index].R.find("R") != string::npos) // Src1 is in ARF
							{
								temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
								index = input.convertStringToInt(temp);
								RSI[i].Vj = RF[index].intRegFile; // copy from ARF to RSI
								//cout << RSI[i].Vj << endl;
							}
						}
						else // Src1 input range is exceeded.
						{
							cout << "Src1 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src1 format
					{
						cout << "Src1 field is wrong, should be in the format of 'Rx'" << endl;;
						exit(1); // exit code.
					}

					// resolving Src2

					if (IQ[pc].Src2.find("R") != string::npos)
					{
						temp = IQ[pc].Src2.substr(1, IQ[pc].Src2.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;
						//Rat[3].R = "ROB6";
						//Rob[6].State = "Write_Back";
						//Rob[6].Value = 57.97;
						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].R.find("ROB") != string::npos) // Src2 is in ROB
							{
								temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;

								// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit")) {
									RSI[i].Vk = (int)Rob[index].Value; // copy from ROB to RSI
									if (RSI[i].Qj == "NULL")
									{
										RSI[i].State = "Ready";
										//cout << RSI[i].State << endl;
									}
									//cout << RSI[i].Vk << endl;
								}
								else // instruction is not finished yet, store ROB entry in Qj.
								{
									RSI[i].Qk = "ROB" + to_string(index);
									//cout << RSI[i].Qj << endl;
									//cout << RSI[i].Qk << endl;
								}
							}
							else if (Rat[index].R.find("R") != string::npos) // Src2 is in ARF
							{
								temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
								index = input.convertStringToInt(temp);
								RSI[i].Vk = RF[index].intRegFile; // copy from ARF to RSI
								if (RSI[i].Qj == "NULL")
								{
									RSI[i].State = "Ready";
								}
							}

						}
						else // Src2 input range is exceeded.
						{
							cout << "Src2 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src2 format
					{
						cout << "Src2 field is wrong, should be in the format of 'Rx'" << endl;
						exit(1); // exit code.
					}
					// store destination in RAT (should be done last because the src and dest might be the same).
					if (IQ[pc].Dest.find("R") != string::npos)
					{
						temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						Rat[index].R = "ROB" + to_string(j); 	// store the ROB destination.
						RSI[i].Dest = "ROB" + to_string(j); 	// store the ROB destination.
					}
					else // wrong Dest format
					{
						cout << "Dest field is wrong, should be in the format of 'Rx'" << endl;
						exit(1); // exit code.
					}
					// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
					pc++;
					Extable[ETable_pc].Instruction = RSI[i].Inst;
					Extable[ETable_pc].Issue = cycles;
					//cout<<Extable[ETable_pc].Instruction << endl;
					//cout << Extable[ETable_pc].Issue << endl;
					ETable_pc++;
					//cout << pc << endl;
					//f = true;
					break;
				}
			}
			break;
			//if (f == true) { break; }
		}
		//break;
	}
	//goto L1;
}

else if (Opcode == "ADDI")
{

	for (int i = 0; i < RSI_entries; i++)
	{
		if (!RSI[i].Busy) // find an empty reservation station.
		{
			for (int j = 0; j < ROB_entries; j++)
			{
				if (!Rob[j].Busy) // find an empty ROB entry
				{
					RSI[i].Busy = true;
					RSI[i].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					RSI[i].Opcode = IQ[pc].Opcode; // may be removed later if of no use.
					RSI[i].ETable_Entry = ETable_pc;
					//cout << RSI[i].Inst << endl;
					Rob[j].Busy = true;
					Rob[j].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					Rob[j].Dest = IQ[pc].Dest; // store the destination in ROB entry.
					Rob[j].Opcode = IQ[pc].Opcode;
					Rob[j].ETable_Entry = ETable_pc;
					Rob[j].State = "NULL";
					//cout << Rob[j].Inst << endl;
					//Rat[2].R = "ROB6";
					//Rob[6].State = "Write_Back";
					//Rob[6].Value = 57.97;
				// resolving Src1
					if (IQ[pc].Src1.find("R") != string::npos)
					{
						temp = IQ[pc].Src1.substr(1, IQ[pc].Src1.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;
						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].R.find("ROB") != string::npos) // Src1 is in ROB
							{
								temp = Rat[index].R.substr(3, Rat[index].R.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;

							// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
								{
									RSI[i].Vj = (int)Rob[index].Value; // copy from ROB to RSI
									RSI[i].State = "Ready";
								}
								else // instruction is not finished yet, store ROB entry in Qj.
								{
									RSI[i].Qj = "ROB" + to_string(index);
								}
							}
							else if (Rat[index].R.find("R") != string::npos) // Src1 is in ARF
							{
								temp = Rat[index].R.substr(1, Rat[index].R.find('R') - 1); // remove R to convert it to integer.
								index = input.convertStringToInt(temp);
								RSI[i].Vj = RF[index].intRegFile; // copy from ARF to RSI
								RSI[i].State = "Ready";
							}
						}
						else // Src1 input range is exceeded.
						{
							cout << "Src1 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src1 format
					{
						cout << "Src1 field is wrong, should be in the format of 'Rx'" << endl;
						exit(1); // exit code.
					}
					// resolving Src2, immediate value, just copy to Vk.

					try
					{
						RSI[i].Vk = input.convertStringToInt(IQ[pc].Src2);
					}
					catch (exception e) // invalid format
					{
						cout << "Src2 field for ADDI operations should be integer" << endl;
						exit(1); // exit code.
					}
					// store destination in RAT (should be done last because the src and dest might be the same).

					if (IQ[pc].Dest.find("R") != string::npos)
					{
						temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('R') - 1); // remove R to convert it to integer.
						index = input.convertStringToInt(temp);
						Rat[index].R = "ROB" + to_string(j); 	// store the ROB destination.
						RSI[i].Dest = "ROB" + to_string(j); 	// store the ROB destination.
					}
					else // wrong Dest format
					{
						cout << "Dest field is wrong, should be in the format of 'Rx'" << endl;
						exit(1); // exit code.
					}

					// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
					pc++;
					Extable[ETable_pc].Instruction = RSI[i].Inst;
					Extable[ETable_pc].Issue = cycles;
					ETable_pc++;
					//cycles++; //new
					break;
				}
				//break;
			}
			break;
		}
		
	}
	//goto L1;
}
// in case of Floating point Addition Operations
else if (Opcode == "ADD.D" || Opcode == "SUB.D")
{
	for (int i = 0; i < RSFA_entries; i++)
	{
		if (!RSFA[i].Busy) // find an empty reservation station.
		{
			for (int j = 0; j < ROB_entries; j++)
			{
				if (!Rob[j].Busy) // find an empty ROB entry
				{
					RSFA[i].Busy = true;
					RSFA[i].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					RSFA[i].Opcode = IQ[pc].Opcode; // may be removed later if of no use.
					RSFA[i].ETable_Entry = ETable_pc;
					//cout << RSFA[i].Inst << endl;
					Rob[j].Busy = true;
					Rob[j].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					Rob[j].Dest = IQ[pc].Dest; // store the destination in ROB entry.
					Rob[j].Opcode = IQ[pc].Opcode;
					Rob[j].ETable_Entry = ETable_pc;
					Rob[j].State = "NULL";

					//cout << Rob[j].Inst << endl;
					//Rat[5].F = "ROB6";
					//cout << Rat[5].F << endl;
					//Rob[6].State = "Write_Back";
					//Rob[6].Value = 57.97;
					// resolving Src1
					if (IQ[pc].Src1.find("F") != string::npos)
					{
						temp = IQ[pc].Src1.substr(1, IQ[pc].Src1.find('F') - 1); // remove F to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;
						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].F.find("ROB") != string::npos) // Src1 is in ROB
							{
								temp = Rat[index].F.substr(3, Rat[index].F.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;

							// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
								{
									RSFA[i].Vj = Rob[index].Value; // copy from ROB to RSFA
									//cout << RSFA[i].Vj << endl;
								}
								else // instruction is not finished yet, store ROB entry in Qj.
								{
									RSFA[i].Qj = "ROB" + to_string(index);
									//cout << RSFA[i].Qj << endl;
								}
							}
							else if (Rat[index].F.find("F") != string::npos) // Src1 is in ARF
							{
								temp = Rat[index].F.substr(1, Rat[index].F.find('F') - 1); // remove F to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;
								RSFA[i].Vj = RF[index].floatRegFile; // copy from ARF to RSFA
								//cout << RSFA[i].Vj << endl;
							}
						}
						else // Src1 input range is exceeded.
						{
							cout << "Src1 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src1 format
					{
						cout << "Src1 field is wrong, should be in the format of 'Fx'" << endl;
						exit(1); // exit code.
					}

					// resolving Src2

					if (IQ[pc].Src2.find("F") != string::npos)
					{
						temp = IQ[pc].Src2.substr(1, IQ[pc].Src2.find('F') - 1); // remove F to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;

						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].F.find("ROB") != string::npos) // Src2 is in ROB
							{
								temp = Rat[index].F.substr(3, Rat[index].F.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;

								// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
								{
									RSFA[i].Vk = Rob[index].Value; // copy from ROB to RSFA
									if (RSFA[i].Qj == "NULL")
									{
										RSFA[i].State = "Ready";
									}
								}
								else // instruction is not finished yet, store ROB entry in Qk.
								{
									RSFA[i].Qk = "ROB" + to_string(index);
								}
							}
							else if (Rat[index].F.find("F") != string::npos) // Src2 is in ARF
							{
								temp = Rat[index].F.substr(1, Rat[index].F.find('F') - 1); // remove F to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;
								RSFA[i].Vk = RF[index].floatRegFile; // copy from ARF to RSFA
								//cout << RSFA[i].Vk << endl;
								if (RSFA[i].Qj == "NULL")
								{
									RSFA[i].State = "Ready";
								}
							}
						}
						else // Src2 input range is exceeded.
						{
							cout << "Src2 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src2 format
					{
						cout << "Src2 field is wrong, should be in the format of 'Fx'" << endl;
						exit(1); // exit code.
					}

					// store destination in RAT (should be done last because the src and dest might be the same).
					if (IQ[pc].Dest.find("F") != string::npos)
					{
						temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('F') - 1); // remove F to convert it to integer.
						index = input.convertStringToInt(temp);
						Rat[index].F = "ROB" + to_string(j); 	// store the ROB destination.
						RSFA[i].Dest = "ROB" + to_string(j); 	// store the ROB destination.
					}

					else // wrong Dest format
					{
						cout << "Dest field is wrong, should be in the format of 'Fx'" << endl;;
						exit(1); // exit code.
					}


					// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
					pc++;
					Extable[ETable_pc].Instruction = RSFA[i].Inst;
					Extable[ETable_pc].Issue = cycles;
					//cout << Extable[ETable_pc].Instruction << endl;
					//cout << Extable[ETable_pc].Issue << endl;
					ETable_pc++;
					break;
				}
				//break;
			}
			break;
		}
		//break;
	}
	//goto L1;
}
else if (Opcode == "MULT.D")
{

	for (int i = 0; i < RSFM_entries; i++)
	{
		if (!RSFM[i].Busy) // find an empty reservation station.
		{
			for (int j = 0; j < ROB_entries; j++)
			{
				if (!Rob[j].Busy) // find an empty ROB entry
				{
					RSFM[i].Busy = true;
					RSFM[i].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					RSFM[i].Opcode = IQ[pc].Opcode; // may be removed later if of no use.
					RSFM[i].ETable_Entry = ETable_pc;
					//cout << RSFM[i].Inst << endl;
					Rob[j].Busy = true;
					Rob[j].Inst = IQ[pc].Opcode + " " + IQ[pc].Dest + " " + IQ[pc].Src1 + " " + IQ[pc].Src2; // store whole instruction for debugging purposes.
					Rob[j].Dest = IQ[pc].Dest; // store the destination in ROB entry.
					Rob[j].Opcode = IQ[pc].Opcode;
					Rob[j].ETable_Entry = ETable_pc;
					Rob[j].State = "NULL";
					//cout << Rob[j].Inst << endl;
					
					// resolving Src1
					if (IQ[pc].Src1.find("F") != string::npos)
					{
						temp = IQ[pc].Src1.substr(1, IQ[pc].Src1.find('F') - 1); // remove F to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;
						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].F.find("ROB") != string::npos) // Src1 is in ROB
							{
								temp = Rat[index].F.substr(3, Rat[index].F.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;
								// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
								{
									RSFM[i].Vj = Rob[index].Value; // copy from ROB to RSFM
								}
								else // instruction is not finished yet, store ROB entry in Qj.
								{
									RSFM[i].Qj = "ROB" + to_string(index);
								}
							}
							else if (Rat[index].F.find("F") != string::npos) // Src1 is in ARF
							{
								temp = Rat[index].F.substr(1, Rat[index].F.find('F') - 1); // remove F to convert it to integer.
								index = input.convertStringToInt(temp);
								RSFM[i].Vj = RF[index].floatRegFile; // copy from ARF to RSFM
							}
						}
						else // Src1 input range is exceeded.
						{
							cout << "Src1 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src1 format
					{
						cout << "Src1 field is wrong, should be in the format of 'Fx'" << endl;
						exit(1); // exit code.
					}
					// resolving Src2

					if (IQ[pc].Src2.find("F") != string::npos)
					{
						temp = IQ[pc].Src2.substr(1, IQ[pc].Src2.find('F') - 1); // remove F to convert it to integer.
						index = input.convertStringToInt(temp);
						//cout << index << endl;
						if (index < RegFiles_entries) // correct input range.
						{
							if (Rat[index].F.find("ROB") != string::npos) // Src2 is in ROB
							{
								temp = Rat[index].F.substr(3, Rat[index].F.find("ROB") - 1); // remove ROB to convert it to integer.
								index = input.convertStringToInt(temp);
								//cout << index << endl;
								// check if this entry is in the write back or commit stage.
								if ((Rob[index].State == "Write_Back") || (Rob[index].State == "Commit"))
								{
									RSFM[i].Vk = Rob[index].Value; // copy from ROB to RSFM
									//cout << RSFM[i].Vk << endl;
									if (RSFM[i].Qj == "NULL")
									{
										RSFM[i].State = "Ready";
									}
								}
								else // instruction is not finished yet, store ROB entry in Qk.
								{
									RSFM[i].Qk = "ROB" + to_string(index);
								}
							}
							else if (Rat[index].F.find("F") != string::npos) // Src2 is in ARF
							{
								temp = Rat[index].F.substr(1, Rat[index].F.find('F') - 1); // remove F to convert it to integer.
								index = input.convertStringToInt(temp);
								RSFM[i].Vk = RF[index].floatRegFile; // copy from ARF to RSFM
								//cout << RSFM[i].Vk << endl;
								if (RSFM[i].Qj == "NULL")
								{
									RSFM[i].State = "Ready";
								}
							}
						}
						else // Src2 input range is exceeded.
						{
							cout << "Src2 exceeded range of register files." << endl;
							exit(1); // exit code.
						}
					}
					else // wrong Src2 format
					{
						cout << "Src2 field is wrong, should be in the format of 'Fx'" << endl;
						exit(1); // exit code.
					}

					// store destination in RAT (should be done last because the src and dest might be the same).
					if (IQ[pc].Dest.find("F") != string::npos)
					{
						temp = IQ[pc].Dest.substr(1, IQ[pc].Dest.find('F') - 1); // remove F to convert it to integer.
						index = input.convertStringToInt(temp);
						Rat[index].F = "ROB" + to_string(j); 	// store the ROB destination.
						RSFM[i].Dest = "ROB" + to_string(j); 	// store the ROB destination.
					}
					else // wrong Dest format
					{
						cout << "Dest field is wrong, should be in the format of 'Fx'" << endl;
						exit(1); // exit code.
					}

					// this point means instruction was correctly fetched an decode, increment pc then, update ExecTable.
					pc++;
					Extable[ETable_pc].Instruction = RSFM[i].Inst;
					Extable[ETable_pc].Issue = cycles;
					ETable_pc++;
					break;
				}
				//break;
			}
			break;
		}
		//break;
	}
	//goto L1;
}
else
{
	cout << "Instruction not recognized" << endl;
	exit(1); // exit code.
}
}
void Execute()
{
	int temp;
	bool brk = false;
	float result;
	bool result_bool;
	bool exec = false;
	int ROB_index;
	//int count = LSQ_count;
	//cout << count << endl;
	LdSdQueue* LSQ_temp = new LdSdQueue();
	//cout << LSQ_count << endl;
	//LSQ.push(*LSQ_temp);
	/**LSQ_temp = LSQ.pop();
	cout << LSQ_temp->Inst << endl;
	for (int i = 0; i < LSQ.d_queue.size(); i++) {
		//*LSQ_temp = LSQ.pop();
			//cout << LSQ_temp->Inst << endl;
		*LSQ_temp=LSQ.d_queue[i];
		LSQ.d_queue[i].Inst = "gj";
		//cout << LSQ_temp->Inst << endl;
	} 
	*LSQ_temp = LSQ.pop();
			cout << LSQ_temp->Inst << endl;
			*LSQ_temp = LSQ.pop();
			cout << LSQ_temp->Inst << endl; */

	if (cycles > 1) // No instruction could start execution before cycle 2.
	{
		// Go for the Integer Functional Unit first

		L2: if (IU_busy) // if we have an instruction which is executing 
		{
			for (int i = 0; i < RSI_entries; i++) 
			{
				if (RSI[i].Busy && (RSI[i].State == "Execute") && !brk) // if we have an instruction and is executed then move to the next FU, because IU is not pipelined
				{
					temp = RSI[i].Cycle + I_cycles; // is it time to write back or not.
					if (temp <= cycles) // if it is time
					{
						string Opcode = RSI[i].Opcode;
						for_each(Opcode.begin(), Opcode.end(), [](char& c) {
							c = ::toupper(c);
							});
						//cout << Opcode << endl;
						if (Opcode == "BEQ")
						{
							result_bool = (RSI[i].Vj == RSI[i].Vk); // get the result.

							if (result_bool)
							{
								pc = pc + RSI[i].Addr;
							}
							Stall = false; // don't stall any more.
							Fetch();
							// empty the ES entry and update the ExecTable.
							RSI[i].Busy = false;
							RSI[i].State = "NULL";
							//ETable[RSI[i].ETable_Entry].WB = cycles;
							IU_busy = false;
							for (int k = 0; k < RSI_entries; k++)
							{
								if (RSI[k].Busy && (RSI[k].State == "Ready") && (Extable[RSI[k].ETable_Entry].Issue < cycles)) // if it is ready and issued prior
								{
									RSI[k].State = "Execute";
									RSI[k].Cycle = cycles;
									Extable[RSI[k].ETable_Entry].Exec = cycles;
									IU_busy = true;
									break;
								}
							}
							// we updated all the things here then break;
							//break;
						}
						else if (Opcode == "BNE") {
							result_bool = (RSI[i].Vj != RSI[i].Vk); // get the result.
							if (result_bool) {
								pc = pc + RSI[i].Addr;
							}

							Stall = false; // don't stall any more.
							Fetch();
							// empty the ES entry and update the ExecTable.
							RSI[i].Busy = false;
							RSI[i].State = "NULL";
							//Extable[RSI[i].ETable_Entry].WB = cycles;
							IU_busy = false;
							for (int k = 0; k < RSI_entries; k++)
							{
								if (RSI[k].Busy && (RSI[k].State == "Ready") && (Extable[RSI[k].ETable_Entry].Issue < cycles)) // if it is ready and issued prior
								{
									RSI[k].State = "Execute";
									RSI[k].Cycle = cycles;
									Extable[RSI[k].ETable_Entry].Exec = cycles;
									IU_busy = true;
									break;
								}
								//break;
							}
							// we updated all the things here then break;
							//break;
							//break;
						}
						else if (Opcode == "ADD" || Opcode == "ADDI")
						{
							if (!CDB_busy)
							{
								CDB_busy = true;  // make the CDB busy
								result = RSI[i].Vj + RSI[i].Vk; // get the result.
								// get the ROB entry that needs update
								string temp = RSI[i].Dest.substr(3, RSI[i].Dest.find("ROB") - 1);
								ROB_index = input.convertStringToInt(temp);
								Rob[ROB_index].Value = (float)result;
								Rob[ROB_index].State = "Commit";

								// find entries that need to be updated in the same RSI
								for (int j = 0; j < RSI_entries; j++)
								{
									if (RSI[j].Busy)
									{
										if (RSI[j].Qj.compare(RSI[i].Dest) == 0) // if any of the Q field match the Dest
										{
											RSI[j].Qj = "NULL";
											RSI[j].Vj = (int)result;
										}

										if (RSI[j].Qk.compare(RSI[i].Dest) == 0) // if any of the Q field match the Dest
										{
											RSI[j].Qk = "NULL";
											RSI[j].Vk = (int)result;
										}

										// if all the operand are ready and the inst is not executing already
										if (!((RSI[j].State == "Execute") || (RSI[j].State == "Ready")))
											if ((RSI[j].Qj == "NULL") && (RSI[j].Qk == "NULL"))
												RSI[j].State = "Ready";
									}
								}
								// find entries that need to be updated in LSQ

								for (int j = 0; j < LSQ_count; j++)
								{
									//*LSQ_temp = LSQ.pop(); // pull entry
									if (LSQ.d_queue[j].Reg.compare(RSI[i].Dest) == 0)
									{
										LSQ.d_queue[j].Reg = to_string(result);
										LSQ.d_queue[j].State = "ReadyA";
									}
									//LSQ.push(*LSQ_temp); // push back again, do this for all entries.
								}
								// empty the ES entry and update the ExecTable.
								RSI[i].Busy = false;
								RSI[i].State = "NULL";
								Extable[RSI[i].ETable_Entry].WB = cycles;
								IU_busy = false;
								// no since an instruction is done, we may start execution of another

								for (int k = 0; k < RSI_entries; k++)
								{
									if (RSI[k].Busy && (RSI[k].State == "Ready") && (Extable[RSI[k].ETable_Entry].Issue < cycles)) // if it is ready and issued prior
									{
										RSI[k].State = "Execute";
										RSI[k].Cycle = cycles;
										Extable[RSI[k].ETable_Entry].Exec = cycles;
										IU_busy = true;
										break;
									}
									//break;
								}
								// we updated all the things here then break;
								break;
							}
						//	break;
						}
						else if (Opcode == "SUB") 
                        {
							if (!CDB_busy)
							{
								CDB_busy = true;  // make the CDB busy
								result = RSI[i].Vj - RSI[i].Vk; // get the result.
								// get the ROB entry that needs update
								string temp = RSI[i].Dest.substr(3, RSI[i].Dest.find("ROB") - 1);
								ROB_index = input.convertStringToInt(temp);
								Rob[ROB_index].Value = (float)result;
								Rob[ROB_index].State = "Commit";

								// find entries that need to be updated in the same RSI

								for (int j = 0; j < RSI_entries; j++)
								{
									if (RSI[j].Busy)
									{
										if (RSI[j].Qj.compare(RSI[i].Dest)==0) // if any of the Q field match the Dest
										{
											RSI[j].Qj = "NULL";
											RSI[j].Vj = (int)result;
										}

										if (RSI[j].Qk.compare(RSI[i].Dest)==0) // if any of the Q field match the Dest
										{
											RSI[j].Qk = "NULL";
											RSI[j].Vk = (int)result;
										}

										// if all the operand are ready and the inst is not executing already										
										if (!((RSI[j].State == "Execute") || (RSI[j].State == "Ready")))
											if ((RSI[j].Qj == "NULL") && (RSI[j].Qk == "NULL"))
												RSI[j].State = "Ready";
									}
								}
								

								// find entries that need to be updated in LSQ

								for (int j = 0; j < LSQ_count; j++)
								{
									//*LSQ_temp = LSQ.pop(); // pull entry
									if (LSQ.d_queue[j].Reg.compare(RSI[i].Dest)==0)
									{
										LSQ.d_queue[j].Reg = to_string(result);
										LSQ.d_queue[j].State = "ReadyA";
									}
									//LSQ.push(*LSQ_temp); // push back again, do this for all entries.
								}
								// empty the ES entry and update the ExecTable.
								RSI[i].Busy = false;
								RSI[i].State = "NULL";
								Extable[RSI[i].ETable_Entry].WB = cycles;
								IU_busy = false;
								// no since an instruction is done, we may start execution of another

								for (int k = 0; k < RSI_entries; k++)
								{
									if (RSI[k].Busy && (RSI[k].State == "Ready") && (Extable[RSI[k].ETable_Entry].Issue < cycles)) // if it is ready and issued prior
									{
										RSI[k].State = "Execute";
										RSI[k].Cycle = cycles;
										Extable[RSI[k].ETable_Entry].Exec = cycles;
										IU_busy = true;
										break;
									}
								}
								// we updated all the things here then break;
								break;
							}
							//break;
                        }

			    		brk = true;
					}
					else // then we are not done yet
						break;
				}
				
			}
			
		}
		// if IU is not busy, search for a ready instruction to execute.
		else
		{
			for (int i = 0; i < RSI_entries; i++)
			{
				if (RSI[i].Busy && (RSI[i].State == "Ready") && (Extable[RSI[i].ETable_Entry].Issue < cycles)) // if it is ready and issued prior
				{
					RSI[i].State = "Execute";
					RSI[i].Cycle = cycles;
					Extable[RSI[i].ETable_Entry].Exec = cycles;
					IU_busy = true;
					break;
					//goto L2;
				}
			}
		}
		// Go for the FP adder Unit

		for (int i = 0; i < RSFA_entries; i++)
		{
			if (RSFA[i].Busy && (RSFA[i].State == "Execute")) // if we have an instruction and is executed then move to the next FU, because IU is not pipelined
			{
				temp = RSFA[i].Cycle + FA_cycles; // is it time to write back or not.
				if (temp <= cycles) // if it is time
				{
					if (!CDB_busy) // and CDB is free
					{
						CDB_busy = true;  // make the CDB busy
						string Opcode = RSFA[i].Opcode;
						for_each(Opcode.begin(), Opcode.end(), [](char& c) {
							c = ::toupper(c);
							});
						//cout << Opcode << endl;

						if (Opcode == "ADD.D")
						{
							result = RSFA[i].Vj + RSFA[i].Vk; // get the result.
							// get the ROB entry that needs update
							string temp = RSFA[i].Dest.substr(3, RSFA[i].Dest.find("ROB") - 1);
							ROB_index = input.convertStringToInt(temp);
							Rob[ROB_index].Value = result;
							Rob[ROB_index].State = "Commit";

							// find entries that need to be updated in the same RSFA

							for (int j = 0; j < RSFA_entries; j++)
							{
								if (RSFA[j].Busy)
								{
									if (RSFA[j].Qj.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFA[j].Qj = "NULL";
										RSFA[j].Vj = result;
									}

									if (RSFA[j].Qk.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFA[j].Qk = "NULL";
										RSFA[j].Vk = result;
									}

									// if all the operand are ready and the inst is not executing already
									if (!((RSFA[j].State == "Execute") || (RSFA[j].State == "Ready")))
										if ((RSFA[j].Qj == "NULL") && (RSFA[j].Qk == "NULL"))
											RSFA[j].State = "Ready";
								}
							}
							// find entries that need to be updated in LSQ
							for (int j = 0; j < LSQ_count; j++)
							{
								//*LSQ_temp = LSQ.pop(); // pull entry
								if (LSQ.d_queue[j].Dest.compare(RSFA[i].Dest) == 0) {
									LSQ.d_queue[j].Dest = to_string(result);
								}
								//LSQ.push(*LSQ_temp); // push back again, do this for all entries.
							}
							// find entries that need to be updated in the RSFM

							for (int j = 0; j < RSFM_entries; j++)
							{
								if (RSFM[j].Busy)
								{
									if (RSFM[j].Qj.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFM[j].Qj = "NULL";
										RSFM[j].Vj = result;
									}

									if (RSFM[j].Qk.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFM[j].Qk = "NULL";
										RSFM[j].Vk = result;
									}

									// if all the operand are ready and the inst is not executing already
									if (!((RSFM[j].State == "Execute") || (RSFM[j].State == "Ready")))
										if ((RSFM[j].Qj == "NULL") && (RSFM[j].Qk == "NULL"))
											RSFM[j].State = "ReadyA";
								}
							}
							// empty the ES entry and update the ExecTable.
							RSFA[i].Busy = false;
							RSFA[i].State = "NULL";
							Extable[RSFA[i].ETable_Entry].WB = cycles;

							// we updated all the things here then break;
							break;
						}
						else if (Opcode == "SUB.D")
						{
							result = RSFA[i].Vj - RSFA[i].Vk; // get the result.

								// get the ROB entry that needs update
							string temp = RSFA[i].Dest.substr(3, RSFA[i].Dest.find("ROB") - 1);
							ROB_index = input.convertStringToInt(temp);
							Rob[ROB_index].Value = result;
							Rob[ROB_index].State = "Commit";
							// find entries that need to be updated in the same RSFA

							for (int j = 0; j < RSFA_entries; j++)
							{
								if (RSFA[j].Busy)
								{
									if (RSFA[j].Qj.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFA[j].Qj = "NULL";
										RSFA[j].Vj = result;
									}

									if (RSFA[j].Qk.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFA[j].Qk = "NULL";
										RSFA[j].Vk = result;
									}

									// if all the operand are ready and the inst is not executing already
									if (!((RSFA[j].State == "Execute") || (RSFA[j].State == "Ready")))
										if ((RSFA[j].Qj == "NULL") && (RSFA[j].Qk == "NULL"))
											RSFA[j].State = "Ready";
								}
							}
							// find entries that need to be updated in LSQ
							for (int j = 0; j < LSQ_count; j++)
							{
								//*LSQ_temp = LSQ.pop(); // pull entry
								if (LSQ.d_queue[j].Dest.compare(RSFA[i].Dest) == 0)
									LSQ.d_queue[j].Dest = to_string(result);
								//LSQ.push(*LSQ_temp); // push back again, do this for all entries.
							}
							// find entries that need to be updated in the RSFM

							for (int j = 0; j < RSFM_entries; j++)
							{
								if (RSFM[j].Busy)
								{
									if (RSFM[j].Qj.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFM[j].Qj = "NULL";
										RSFM[j].Vj = result;
									}

									if (RSFM[j].Qk.compare(RSFA[i].Dest) == 0) // if any of the Q field match the Dest
									{
										RSFM[j].Qk = "NULL";
										RSFM[j].Vk = result;
									}

									// if all the operand are ready and the inst is not executing already
									if (!((RSFM[j].State == "Execute") || (RSFM[j].State == "Ready")))
										if ((RSFM[j].Qj == "NULL") && (RSFM[j].Qk == "NULL"))
											RSFM[j].State = "ReadyA";
								}
							}
							// empty the ES entry and update the ExecTable.
							RSFA[i].Busy = false;
							RSFA[i].State = "NULL";
							Extable[RSFA[i].ETable_Entry].WB = cycles;

							// we updated all the things here then break;
							break;
						}
						// one instruction has write back so break, no need to loop.
						break;
					}
					else // CDB is not free
					{
						break;
					}
				}
			}
			else // if instruction is ready make it execute because it is piplined
			{
				if (RSFA[i].Busy && (RSFA[i].State == "Ready") && (Extable[RSFA[i].ETable_Entry].Issue < cycles)) // if it is ready and issued prior) // if it is ready
				{
					RSFA[i].State = "Execute";
					RSFA[i].Cycle = cycles;
					Extable[RSFA[i].ETable_Entry].Exec = cycles;
					break;
				}
			}
		}
		// Go for the FP Multiplier Unit
		for (int i = 0; i < RSFM_entries; i++)
		{
			if (RSFM[i].Busy && (RSFM[i].State == "Execute")) // if we have an instruction and is executed then move to the next FU, because IU is not pipelined
			{
				temp = RSFM[i].Cycle + FM_cycles; // is it time to write back or not.
				if (temp <= cycles) // if it is time
				{
					if (!CDB_busy) // and CDB is free
					{
						CDB_busy = true;  // make the CDB busy
						string Opcode = RSFM[i].Opcode;
						for_each(Opcode.begin(), Opcode.end(), [](char& c) {
							c = ::toupper(c);
							});
						//cout << Opcode << endl;
						if (Opcode == "MULT.D")
						{
							result = RSFM[i].Vj * RSFM[i].Vk; // get the result.

								// get the ROB entry that needs update
							string temp = RSFM[i].Dest.substr(3, RSFM[i].Dest.find("ROB") - 1);
							ROB_index = input.convertStringToInt(temp);
							Rob[ROB_index].Value = result;
							Rob[ROB_index].State = "Commit";
							// find entries that need to be updated in the same RSFM

							for (int j = 0; j < RSFM_entries; j++)
							{
								if (RSFM[j].Busy)
								{
									if (RSFM[j].Qj.compare(RSFM[i].Dest)==0) // if any of the Q field match the Dest
									{
										RSFM[j].Qj = "NULL";
										RSFM[j].Vj = result;
									}

									if (RSFM[j].Qk.compare(RSFM[i].Dest)==0) // if any of the Q field match the Dest
									{
										RSFM[j].Qk = "NULL";
										RSFM[j].Vk = result;
									}

									// if all the operand are ready and the inst is not executing already
									if (!((RSFM[j].State == "Execute") || (RSFM[j].State == "Ready")))
										if ((RSFM[j].Qj == "NULL") && (RSFM[j].Qk == "NULL"))
											RSFM[j].State = "ReadyA";
								}
							}
							// find entries that need to be updated in LSQ
							for (int j = 0; j < LSQ_count; j++)
							{
								//*LSQ_temp = LSQ.pop(); // pull entry
								if (LSQ.d_queue[j].Dest.compare(RSFM[i].Dest)==0)
									LSQ.d_queue[j].Dest = to_string(result);
								//LSQ.push(*LSQ_temp); // push back again, do this for all entries.
							}
							// find entries that need to be updated in the RSFA

							for (int j = 0; j < RSFA_entries; j++)
							{
								if (RSFA[j].Busy)
								{
									if (RSFA[j].Qj.compare(RSFM[i].Dest)==0) // if any of the Q field match the Dest
									{
										RSFA[j].Qj = "NULL";
										RSFA[j].Vj = result;
									}

									if (RSFA[j].Qk.compare(RSFM[i].Dest)==0) // if any of the Q field match the Dest
									{
										RSFA[j].Qk = "NULL";
										RSFA[j].Vk = result;
									}

									// if all the operand are ready and the inst is not executing already
									if (!((RSFA[j].State == "Execute") || (RSFA[j].State == "Ready")))
										if ((RSFA[j].Qj == "NULL") && (RSFA[j].Qk == "NULL"))
											RSFA[j].State = "Ready";
								}
							}
							// empty the ES entry and update the ExecTable.
							RSFM[i].Busy = false;
							RSFM[i].State = "NULL";
							Extable[RSFM[i].ETable_Entry].WB = cycles;

							// we updated all the things here then break;
							break;

						}
						// one instruction has write back so break, no need to loop.
						break;
					}
					else // CDB is not free
					{
						break;
					}
				}
			}
			else // if instruction is ready make it execute, because it is piplined
			{
				if (RSFM[i].Busy && (RSFM[i].State == "Ready") && (Extable[RSFM[i].ETable_Entry].Issue < cycles)) // if it is ready and issued prior
				{
					RSFM[i].State = "Execute";
					RSFM[i].Cycle = cycles;
					Extable[RSFM[i].ETable_Entry].Exec = cycles;

					for (int j = i + 1; j < RSFM_entries; j++)
						if (RSFM[i].Busy && (RSFM[i].State == "ReadyA")) // just make it ready for the next cycle.
							RSFM[i].State = "Ready";

					break;
				}
				else if (RSFM[i].Busy && (RSFM[i].State == "ReadyA")) // just make it ready for the next cycle.
					RSFM[i].State = "Ready";
			}
		}
		// Go for the Load Store Functional unit

		for (int j = 0; j < LSQ_count; j++) // look for any instruction that is executing.
		{
			//*LSQ_temp = LSQ.pop(); // pull entry

			if ((LSQ.d_queue[j].State == "Execute"))
			{
				temp = LSQ.d_queue[j].Cycle + 1; // is execution finished or not.
				if (temp == cycles)
				{
					result = (float)LSQ.d_queue[j].Offset + (int)input.convertStringToFloat(LSQ.d_queue[j].Reg);
					LSQ.d_queue[j].Addr = (int)result;
					// store also in the ROB.dest
					for (int l = 0; l < ROB_entries; l++)
					{
						if (Rob[l].Busy && (Rob[l].ETable_Entry == LSQ.d_queue[j].ETable_Entry))
						{
							Rob[l].Addr = (int)result;
							break;
						}
					}
					LSQ.d_queue[j].State = "Memory";
					//LSQ.push(*LSQ_temp);
					break;
				}
			}
		//	LSQ.push(*LSQ_temp);
		}
		for (int j = 0; j < LSQ_count; j++) // look for any instruction that is ready.
		{
			//*LSQ_temp = LSQ.pop(); // pull entry

			if ((LSQ.d_queue[j].State == "Ready") && (Extable[LSQ.d_queue[j].ETable_Entry].Issue < cycles))
			{
				LSQ.d_queue[j].State = "Execute";
				LSQ.d_queue[j].Cycle = cycles;
				Extable[LSQ.d_queue[j].ETable_Entry].Exec = cycles;
				//LSQ.push(*LSQ_temp);
				break;
			}
			//LSQ.push(*LSQ_temp);

		}
		
		for (int j = 0; j < LSQ_count; j++) // look for any instruction that is ready.
		{
			//*LSQ_temp = LSQ.pop(); // pull entry
			if (LSQ.d_queue[j].State == "ReadyA") {
				LSQ.d_queue[j].State = "Ready";
			}
			//LSQ.push(*LSQ_temp);
		}

	}
}
void memory() 
{
	string temp;
	int index;
	double val;
	bool match_found = false;
	bool fill_up = false;
	LdSdQueue* LSQ_temp = new LdSdQueue();
	LdSdQueue* LSQ_temp1 = new LdSdQueue();
	int count = LSQ_count;

	

	if (Mem_busy && Mem_busy_sd) {
		if ((Cycle_Mem_busy + LS_cycles_mem) == cycles)
		{
			Mem_busy = false;
			Mem_busy_sd = false;
		}
	}
	if (count == 0)
		return;
	if (LSQ_count == 0)
		return ;
	queue<LdSdQueue> *LSQ_local = new queue<LdSdQueue>[LSQ_count];
	//ArrayBlockingQueue<LoStQu_Struct> LSQ_local = new ArrayBlockingQueue<LoStQu_Struct>(LSQ_count);
	if (Mem_busy) // if memory is busy
	{
		if ((Cycle_Mem_busy + LS_cycles_mem) <= cycles) // memory is done
		{
			int LSQ_size = LSQ_count;
			for (int i = 0; i < LSQ_size; i++) 
			{
				*LSQ_temp = LSQ.pop();
				if (!fill_up)
				{
					if ((LSQ_temp->State.compare("InMemory")==0) && !CDB_busy) // if Instruction already in memory and CDB is free
					{
						CDB_busy = true;
						Mem_busy = false;

						val = (float)Mem[LSQ_temp->Addr].Value;
						// get the ROB entry that needs update
						Rob[LSQ_temp->ROB_entry].Value = val;
						Rob[LSQ_temp->ROB_entry].State = "Commit";

						// find entries that need to be updated in the same RSFA

						for (int j = 0; j < RSFA_entries; j++)
						{
							if (RSFA[j].Busy)
							{
								if (RSFA[j].Qj.compare(LSQ_temp->Dest)==0) // if any of the Q field match the Dest
								{
									RSFA[j].Qj = "NULL";
									RSFA[j].Vj = val;
								}

								if (RSFA[j].Qk.compare(LSQ_temp->Dest)==0) // if any of the Q field match the Dest
								{
									RSFA[j].Qk = "NULL";
									RSFA[j].Vk = val;
								}

								// if all the operand are ready and the inst is not executing already
								if (!((RSFA[j].State == "Execute") || (RSFA[j].State == "Ready")))
									if ((RSFA[j].Qj == "NULL") && (RSFA[j].Qk == "NULL"))
										RSFA[j].State = "Ready";
							}
						}
						// find entries that need to be updated in the RSFM

						for (int j = 0; j < RSFM_entries; j++)
						{
							if (RSFM[j].Busy)
							{
								if (RSFM[j].Qj.compare(LSQ_temp->Dest)==0) // if any of the Q field match the Dest
								{
									RSFM[j].Qj = "NULL";
									RSFM[j].Vj = val;
								}

								if (RSFM[j].Qk.compare(LSQ_temp->Dest)==0) // if any of the Q field match the Dest
								{
									RSFM[j].Qk = "NULL";
									RSFM[j].Vk = val;
								}

								// if all the operand are ready and the inst is not executing already
								if (!((RSFM[j].State == "Execute") || (RSFM[j].State == "Ready")))
									if ((RSFM[j].Qj == "NULL") && (RSFM[j].Qk == "NULL"))
										RSFM[j].State = "Ready";
							}
						}

						// empty the ES entry and update the ExecTable.
						Extable[LSQ_temp->ETable_Entry].WB = cycles;

						// check if any other instruction 

						fill_up = true;

						LSQ_count--;
					}
					else
						LSQ.push(*LSQ_temp);
				}
				else
					LSQ.push(*LSQ_temp);
			}
			// check if CDB is free do its code
			if (!Mem_busy)
			{
				fill_up = false;
				for (int i = 0; i < LSQ_count; i++)
				{
					*LSQ_temp = LSQ.pop();
					LSQ_local[i].push(*LSQ_temp); // copy each entry to another queue to check for dependencies
					if( (LSQ_temp->State.compare("Memory")==0) && !fill_up) // if Instruction in memory stage and not fill up
					{
						string Opcode = LSQ_temp->Opcode;
						for_each(Opcode.begin(), Opcode.end(), [](char& c) {
							c = ::toupper(c);
							});
						//cout << Opcode << endl;
						if (Opcode == "LD") 
						{
							if (!match_found) // if I didn't find a match or no instructions are ready, then go to memory
							{
								Mem_busy = true;
								Cycle_Mem_busy = cycles;
								LSQ_temp->State = "InMemory";
								LSQ.push(*LSQ_temp);
								Extable[LSQ_temp->ETable_Entry].Mem = cycles;
								fill_up = true;
							}
							break;
						}
						else if (Opcode == "SD") 
						{
							try
							{
								// if value is ready then yous hsould be able to commit
								val = input.convertStringToFloat(LSQ_temp->Dest);

								Rob[LSQ_temp->ROB_entry].Value = val;
								Rob[LSQ_temp->ROB_entry].Addr = LSQ_temp->Addr;
								LSQ_temp->State = "Commit";
								Rob[LSQ_temp->ROB_entry].State = "Commit";
								fill_up = true;
								LSQ_count--;
							}
							catch (exception e) // then value is not ready yet
							{
								LSQ.push(*LSQ_temp);
							}

							break;
						}

					}
					else // if not in memory stage, put it back to the queue
						LSQ.push(*LSQ_temp);
				}
			}

		}
	}
	else // if memory is free
	{
		for (int i = 0; i < LSQ_count; i++)
		{
			*LSQ_temp = LSQ.pop();
			LSQ_local->push(*LSQ_temp); // copy each entry to another queue to check for dependencies
			if ((LSQ_temp->State.compare("Memory")==0) && !fill_up) // if Instruction in memory stage and not fill up
			{
				string Opcode = LSQ_temp->Opcode;
				for_each(Opcode.begin(), Opcode.end(), [](char& c) {
					c = ::toupper(c);
					});
				//cout << Opcode << endl;
				if (Opcode == "LD") {
					if (!match_found) // if I didn't find a match or no instructions are ready, then go to memory
					{
						Mem_busy = true;
						Cycle_Mem_busy = cycles;
						LSQ_temp->State = "InMemory";
						LSQ.push(*LSQ_temp);
						Extable[LSQ_temp->ETable_Entry].Mem = cycles;
						fill_up = true;
					}
					break;

				}
				else if (Opcode == "SD") {
					try
					{
						// if value is ready then yous hsould be able to commit
						val = input.convertStringToFloat(LSQ_temp->Dest);

						Rob[LSQ_temp->ROB_entry].Value = val;
						Rob[LSQ_temp->ROB_entry].Addr = LSQ_temp->Addr;
						LSQ_temp->State = "Commit";
						Rob[LSQ_temp->ROB_entry].State = "Commit";
						LSQ_count--;
						fill_up = true;
					}
					catch (exception e) // then value is not ready yet
					{
						LSQ.push(*LSQ_temp);
					}
					break;
				}
			}
			else // if not in memory stage, put it back to the queue
				LSQ.push(*LSQ_temp);
		}
	}
}
void commit()
{
	bool temp;
	int index;
	string test;
	test = Extable[inst_commit].Instruction;
	if ((test.find("BNE")!= string::npos) || (test.find("BEQ")!= string::npos) )// if next instruction is Branch then increment inst_commit
		inst_commit++; //instruction that should commit

	for (int i = 0; i < ROB_entries; i++)
	{

		// if entry is busy and instruction is ready to commit and in order
		temp = Rob[i].Busy && (Rob[i].State.compare("Commit")==0) && (Rob[i].ETable_Entry == inst_commit);
		temp = temp && (Extable[Rob[i].ETable_Entry].WB < cycles);
		if (temp)
		{
			string Opcode = Rob[i].Opcode;
			for_each(Opcode.begin(), Opcode.end(), [](char& c) {
				c = ::toupper(c);
				});
			//cout << Opcode << endl;
			if (Opcode == "ADD" || Opcode == "SUB" || Opcode == "ADDI") 
			{
				string x = Rob[i].Dest.substr(1, Rob[i].Dest.find('R') - 1); // remove R to convert it to integer.
				index = input.convertStringToInt(x);
				RF[index].intRegFile = (int)Rob[i].Value; // update register file.
				// check whether to update the RAT or not.
				if (Rat[index].R.compare("ROB" + to_string(i))==0)
					Rat[index].R = "R" + to_string(index);
			}
			else if (Opcode == "ADD.D" || Opcode == "SUB.D" || Opcode == "MULT.D" || Opcode == "LD") 
			{
				string x = Rob[i].Dest.substr(1, Rob[i].Dest.find('F') - 1); // remove F to convert it to integer.
				index = input.convertStringToInt(x);
				RF[index].floatRegFile = Rob[i].Value; // update register file.
			}
			else if (Opcode == "SD") 
			{
				if (Mem_busy)
					return;

				index = Rob[i].Addr; // get index of register file
				Mem[index].Value = Rob[i].Value; // update register file.

				Mem_busy = true;
				Cycle_Mem_busy = cycles;
				Mem_busy_sd = true;
			}
			Rob[i].Busy = false;
			Rob[i].State = "NULL";
			Extable[Rob[i].ETable_Entry].Commit = cycles;
			inst_commit++;
			break;
		}
		//break;
	}
}


//----------------------------------------------------------------------------------------------------------------------
void intialize() {
	//creat inst. buffer, Register file, ROB and intialize parameters needed
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
	for (int i = 0; i < ROB_entries; i++)
	{
		//Rob = new ROB[i]; 		// initialize each entry in the ROB.
		Rob[i].Busy = false;
	}
	//-----------------------------------------------------------------------------------------------------
   // creating our instruction buffer

	IQ = new InstBuffer[IQ_entries]; // pointer to instruction buffer

	for (int i = 0; i < IQ_entries; i++) {
		vector <string> temp = input.ReadInstruction(input.instructions[i]); //we know that temp size is always 4
		(*(IQ + i)).Opcode = temp[0];
		(*(IQ + i)).Dest = temp[1];
		//(*(Rob + i)).Dest = temp[1];
		(*(IQ + i)).Src1 = temp[2];
		(*(IQ + i)).Src2 = temp[3];
		//output test--------------------------------------------------------------------------------
	/*
		cout << "opcode " << i << (*(IQ + i)).Opcode << endl;
		cout << "Dest " << i << (*(IQ + i)).Dest << endl;
		cout<< "dest rob "<< i << (*(Rob +i)).Dest <<endl;
		cout << "src1 " << i << (*(IQ + i)).Src1 << endl;
		cout << "src2 " << i << (*(IQ + i)).Src2 << endl;  */
		//------------------------------------------------------------------------------------------
	}
	//---------------------------------------------------------------------------------------------------------
	//intializing the 32 registers to 0
	for (int i = 0; i < RegFiles_entries; i++) {
		RF[i].intRegFile = 0;
		RF[i].floatRegFile = 0.0;
	}
	// every register is now hardwired to it's number R0 is at RF[0] ans so on

	for (int i = 0; i < input.register_var.size(); i++)
	{ // my format is R1=10 , F2=30.1 ... etc will get which reg
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

	//cout << RF[7].intRegFile << endl;
	//cout << RF[9].floatRegFile << endl;
	//cout << "R1 " << RF[1].intRegFile << " " << " R2 " << RF[2].intRegFile << " F2 " << RF[2].floatRegFile << endl;
	//----------------------------------------------------------------------------------------------------------------


}


//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------****Main program****-------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
int main()
{
	intialize();
	ETable_entries = 1000;
	Arch_unit();
	
	bool ex = true;
	
	do
	{

		if (!Stall)
		Fetch();

		Execute();
		memory();

		CDB_busy = false;

		commit();

		//cout<<Integer.toString(LSQ.remainingCapacity()) + "   " + cycles + "\n");
		cycles++;

		if (dont_fetch)
		{
			ex = false;
			for (int i = 0; i < ROB_entries; i++)
				if (Rob[i].Busy)
				{
					ex = true;
					//break;
				}
		}
		
	} 
	while (ex);


	Output o;
	o.OutputFcn(Extable, RF, Mem,input); 
	//memory();
	/*LdSdQueue* Lsq = new LdSdQueue();
	*Lsq = LSQ.pop();
	cout << Lsq->Inst << endl; // just a test for queue class
	*Lsq = LSQ.pop();
	cout << Lsq->Inst << endl; // just a test for queue class */
	//commit();
	//cout << IQ[0].Dest << endl;
	//cout << "R0 "<<RF[0].intRegFile<<" R1 " << RF[1].intRegFile << " " << " R2 " << RF[2].intRegFile << " F2 " << RF[2].floatRegFile << endl;
	//cout << Rat[0].R << endl;
	//cout << Rat[1].R << endl;
	//cout << Rat[1].F << endl;
	//cout << "Mem[1] " << Mem[1].Value << " Mem[4] " << Mem[4].Value << " Mem[12] " << Mem[12].Value << endl;
}

