#include <iostream>
#include <string>
using namespace std;
class ExeTable // Structure definition for the execution table.
{
public:
    string Instruction;
    int Issue;
    int Exec;
    int Mem;
    int WB;
    int Commit;
};