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
int main()
{   
	Import read;
	read.ReadInput();
	read.SetVariables();
    //std::cout << "Hello World!\n";
}

