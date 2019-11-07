#ifndef InstBuffer_h
#define InstBuffer_h
#include <iostream>
#include <string>
using namespace std;
class InstBuffer
{
public:
	InstBuffer(){}
    string Opcode;
    string Dest;
    string Src1;
    string Src2;
};
#endif