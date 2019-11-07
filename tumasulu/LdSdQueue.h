#ifndef LdSdQueue_h
#define LdSdQueue_h
#include <iostream>
#include <string>
using namespace std;
class LdSdQueue
{ public:
    string Dest;      // Destination of the load operation.
    int Offset;       // offset of memory address for Ld/St.
    string Reg;       // Register of memory address for Ld/St.
    string Inst;      // Instruction field.
    string Opcode;    // Operand field.
    string State;     // State of the instruction (Ready, Execute).
    int Addr;         // Address after calculation in Execution stage.
    int Cycle;        // Cycle started execution.
    int ETable_Entry; // ETable entry for that instruction.
    int ROB_entry;    // entry of ROB for SD.
};
#endif