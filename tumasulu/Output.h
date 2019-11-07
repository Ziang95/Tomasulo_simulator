#ifndef OUTPUT_H
#define OUTPUT_H
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
#include "Memory.h"
#include "ROB.h"
#include "ExeTable.h"
#include "LdSdQueue.h"
#include "Import.h"
#include "queue.h"

using namespace std;
class Output {

public:
	void OutputFcn(ExeTable *Extable, RegFiles *RFiles, Memory *Mem)
	{
		Import in;
	// This function print the Output file after the code execution.
	 // it should be called from the main program, after the code execution, to make the output file.
		try
		{
			ofstream outfile("extable.txt");
			// Output file where the data would Execution table, register files and memory would be written.
			//FileWriter writer = new FileWriter("Output File.csv");


			// First we print the header of the execution table.
			outfile<<"Execution Table" ;
			outfile << endl;
			outfile<<"Instructions,ISSUE,EXEC,MEM,WB,COMMIT \n";

			// then we print the execution table itself.
			for (int i = 0; i <9; i++)
			{
				if (Extable[i].Instruction != "")
				{
					outfile<<Extable[i].Instruction + ',' + "       ";
					if (Extable[i].Issue != 0)
						outfile<<(to_string(Extable[i].Issue) + ',');
					else
						outfile<<(" ,");

					if (Extable[i].Exec != 0)
						outfile<<(to_string(Extable[i].Exec) + ',');
					else
						outfile<<(" ,");

					if (Extable[i].Mem != 0)
						outfile<<(to_string(Extable[i].Mem) + ',');
					else
						outfile << (" ,");
					if (Extable[i].WB != 0)

						outfile << (to_string(Extable[i].WB) + ',');
					else
						outfile << (" ,");

					if (Extable[i].Commit != 0)
						outfile << (to_string(Extable[i].Commit) + '\n');
					else
						outfile << (" \n");

				}
			}

			outfile << ("\n\n");
/*
			// Second we print the header of the Integer Registers Files.
			outfile << ("Integer Register File \n");
			outfile << ("REGs,VALUE,REGs,VALUE \n");

			// then we print the Integer Register File themselves.
			for (int i = 0; i < 32 ; i = i + 2)
				outfile << ("R" + i + ',' + to_string(RFiles[i].intRegFile) + ',' + "R" + to_string((i + 1)) + ',' + to_string(RFiles[i + 1].intRegFile) + '\n');

			outfile << ("\n\n");

			// Third we print the header of the Floating point Registers Files.
			outfile << ("Floating point Register File \n");
			outfile << ("REGs,VALUE,REGs,VALUE \n");

			// then we print the Floating point Register File themselves.
			for (int i = 0; i < 32; i = i + 2)
				outfile<<("F" + i + ',' + to_string(RFiles[i].floatRegFile) + ',' + "F" + to_string((i + 1)) + ',' + to_string(RFiles[i + 1].floatRegFile) + '\n');

			outfile << ("\n\n");

			// Finally we print the header of the Memory.
			outfile << ("Non-Zero Memory Values \n");
			outfile << ("ADDRESS,VALUE \n");

			// then we print the Memory values themselves.
			string temp;
			for (int i = 0; i < 64; i++)
			{
				temp = to_string(i) + ',';
				temp = temp + to_string(Mem[i].Value);
				temp = temp + '\n';;
				outfile << (temp);
			} */

			outfile.flush();
			outfile.close();
		}
		catch (exception e)
		{
			//e.printStackTrace();
		} 
	}

};
#endif