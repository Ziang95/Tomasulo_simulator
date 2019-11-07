#ifndef ROB_H
#define ROB_H
#include <iostream>
#include <string>
using namespace std;
class ROB
{
public:
    bool Busy;        // Ready to commit or not.
	string Dest;      // Destination of the instruction (R1, R2, F1, F2)
	double Value;     // Result of the instruction, entry of ROB waiting to finish, address for Ld/St.
	string Opcode;    // Opcode of the operation.
    string Inst;      // Instruction field.
    string State;     // State of the instruction (Execute, Write_Result, Commit).
    int Addr;         // Adress for load and store.
    int ETable_Entry; // ETable entry for that instruction.
};
#endif