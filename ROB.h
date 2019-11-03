#include <iostream>
#include <string>

using namespace std;

struct timeLine
{
    int issue;
    int exe;
    int mem;
    int commit;
};

class ROBEntry
{
public:
    bool finished;      // Ready to commit or not.
    string name;
    string Dest;        // Destination of the instruction (R1, R2, F1, F2)
    memCell Value;      // Result of the instruction, entry of ROB waiting to finish, address for Ld/St.
    opCode code;        // Opcode of the operation.
    timeLine output;

    string Inst;        // Instruction field.
    string State;       // State of the instruction (Execute, Write_Result, Commit).
    int Addr;           // Address for load and store.
    int ETable_Entry;   // ETable entry for that instruction.
};

class ROB
{
    private:
        int front, rear;
    public:
        const int size;
        ROBEntry *buf;
        ROB(int size);
        ROBEntry* get_entry(int index);
        int add_entry();
        void ROB_automate();
};