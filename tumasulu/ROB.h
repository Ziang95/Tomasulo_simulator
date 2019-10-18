#include <iostream>
#include <string>
using namespace std;
class ROB
{
public:
    bool Busy;        // true for busy and false for free.
    string Inst;      // Instruction field.
    string State;     // State of the instruction (Execute, Write_Result, Commit).
    string Dest;      // Destination of the instruction (FPregs, InRegs, Source of stores also).
    double Value;     // Result of the instruction, entry of ROB waiting to finish, address for Ld/St.
    string Opcode;    // Opcode of the operation.
    int Addr;         // Adress for load and store.
    int ETable_Entry; // ETable entry for that instruction.
};