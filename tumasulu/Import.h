#ifndef IMPORT_H
#define IMPORT_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

class Import
{
	// this class is used to read input.txt and give back all the info needed to start.
private:
	vector<string> Inst_content;
	int number_of_lines = 0;
	vector<string> input;

public:
	int Int_adder_rs;
	int Int_adder_ex;
	int Int_adder_fu;
	int FP_adder_rs;
	int FP_adder_ex;
	int FP_adder_fu;
	int FP_Mul_rs;
	int FP_Mul_ex;
	int FP_Mul_fu;
	int LdSd_rs;
	int LdSd_ex;
	int LdSd_mem;
	int LdSd_fu;
	int ROB_entries;
	vector<string> instructions;
	vector<string> memory_var;
	vector<string> register_var;
	void ReadInput();// read input file and remove extra spaces and commas
	int convertStringToInt(string& input); // Convert string to int
	double convertStringToFloat(string& input); //convert string to float
	vector<string> ReadInstruction(string str); //geting opccode, dest, src1, src2
	void SetVariables();//read data line by line and get unique info from each line. input format is constant just reg value and memory will increase

};
#endif