#include <iostream>
#include <string>
using namespace std;
class RsFU
{
public:
    string Inst;      // Instruction field.
    bool Busy;        // free or not.
    string Opcode;    // Operand field .
    string State;     // State of the instruction (Ready, Execute).
    int Cycle;        // Cycle started execution.
    double Vj;           // value of the first operand.
    double Vk;           // value of the second operand.
    string Qj;        // name from RAt table for the first operand.
    string Qk;        // name from RAt table for the second operand.
    int Addr;         // offset of branch instructions.
    string Dest;      // ROB entry that will store the operand.
    int ETable_Entry; // ETable entry for that instruction.
};