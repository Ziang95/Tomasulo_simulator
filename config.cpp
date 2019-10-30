#include "config.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

void config::ReadInput()
{
	// readind input text file
	string s;
	ifstream infile;
	infile.open("InputTest.txt");
	//ofstream outfile;
	ofstream outfile("test.txt");
	if (outfile.fail())
	{
		cout << "Output file could not be opened.\n";
		exit(1);
	}

	if (!infile)
	{
		cerr << "Unable to open file datafile.txt";
		exit(1); // call system to stop
	}

L1:while (getline(infile, s)) {
	if (!s.empty()) {
		stringstream line(s);
		while (line >> s) {
			outfile << s;
			if (s != ",") {
				input.push_back(s);
				outfile << ' ';
			}
		}
	}
	else {
		goto L1;
	}
	outfile << endl; // put input in a new file after removing all extra spaces
	//------------------------------------------------------------------------------------
	number_of_lines++;
	//cout << s << '\n' << endl;		
}
/*for (int i = 0; i < input.size(); i++)
	cout << input[i] << "\n"; */
infile.close();
outfile.close();
}
//----------------------------------------------------------------------------
int config::convertStringToInt(string& input)
{
	// Convert string to int

	stringstream ss(input);
	int retval;
	ss >> retval;
	return retval;
}
//--------------------------------------------------------------------------------
double config::convertStringToFloat(string& input)
{
	//convert string to float

	stringstream ss(input);
	double retval;
	ss >> retval;
	return retval;
}
//---------------------------------------------------------------------------------
vector<string> config::ReadInstruction(string str)
{
	//geting opccode, dest, src1, src2
	// Used to split string around spaces. 
	istringstream ss(str);
	Inst_content.clear();
	// Traverse through all words 
	do {
		// Read a word 
		string word;
		ss >> word;

		// push to vector the read word 
		Inst_content.push_back(word);
		// While there is more to read 
	} while (ss);
	return Inst_content;
}

void config::SetVariables() {
	ifstream infile;
	string s;
	vector<string> line1;
	int no_lines = 0;
	infile.open("test.txt");
	if (!infile)
	{
		cerr << "Unable to open file datafile.txt";
		exit(1); // call system to stop
	}

	while (getline(infile, s)) {
		//read data line by line and get unique info from each line. input format is constant just reg value and memory will increase
		if (no_lines == 1) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			Int_adder_rs = convertStringToInt(line1[2]);
			Int_adder_ex = convertStringToInt(line1[3]);
			Int_adder_fu = convertStringToInt(line1[4]);
			line1.clear();
		}

		if (no_lines == 2) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			FP_adder_rs = convertStringToInt(line1[2]);
			FP_adder_ex = convertStringToInt(line1[3]);
			FP_adder_fu = convertStringToInt(line1[4]);
			line1.clear();
		}

		if (no_lines == 3) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			FP_Mul_rs = convertStringToInt(line1[2]);
			FP_Mul_ex = convertStringToInt(line1[3]);
			FP_Mul_fu = convertStringToInt(line1[4]);
			line1.clear();
		}

		if (no_lines == 4) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			LdSd_rs = convertStringToInt(line1[2]);
			LdSd_ex = convertStringToInt(line1[3]);
			LdSd_mem = convertStringToInt(line1[4]);
			LdSd_fu = convertStringToInt(line1[5]);

			line1.clear();
		}

		if (no_lines == 5) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			ROB_entries = convertStringToInt(line1[3]);

			line1.clear();
		}

		if (no_lines == 7) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			for (int i = 0; i < line1.size(); i++) { //line1 nw contains all reg name and value with delimter ',' I am removing it to make the string
				//only conatins variable name before '=' and variable value after equal.
				//string delimiter = "=";
				//string token= line1[i].substr(0, line1[i].find(delimiter)); //token1 is R1, R2 ,F2 ,.. etc
				//line1[i].erase(remove(line1[i].begin(), line1[i].end(), '='), line1[i].end());
				line1[i].erase(remove(line1[i].begin(), line1[i].end(), ','), line1[i].end());
				register_var.push_back(line1[i]);
			}

			//if want to find first reg value can get it like this but for now I think it's better to leave the string as R1=10 until later on
			// just test to make sure I can get the value correct. if float use function convertStringtofloat and vise versa.
			/*int length = register_var[3].length();
			size_t pos = register_var[3].find('=');
			cout << pos << endl;
			string token = register_var[3].substr(pos+1, length);
			cout << token << endl;
			double Reg1value = convertStringToFloat(token);
			cout << " first register value is " << Reg1value << endl;
			for (int i = 0; i < register_var.size(); i++) // just to check what is in reg_var now
				cout << register_var[i] << "\n";*/

			line1.clear();
		}

		// for now format Mem[4]=10
		if (no_lines == 8) {
			stringstream line(s);
			while (line >> s) {
				line1.push_back(s);
			}
			for (int i = 0; i < line1.size(); i++) {
				line1[i].erase(remove(line1[i].begin(), line1[i].end(), ','), line1[i].end());
				memory_var.push_back(line1[i]);
			}
			line1.clear();
		}

		// starting from line 9 it will be instructions untill number_of_lines
		// removing {, ( ) } so now every line of inst is sperated be a space delimter
		// format: Opcode DestReg src1 src2     if we found it's better to change it later we will.
		if (no_lines >= 9) {
			s.erase(remove(s.begin(), s.end(), ','), s.end());
			replace(s.begin(), s.end(), '(', ' ');
			replace(s.begin(), s.end(), ')', ' ');
			instructions.push_back(s);

		}

		no_lines++;
	}
	//output to check compare with .txt 
	//------------------------------------------------------------------------------------------------------
	/*
	cout << "Number of lines in text file: " << number_of_lines << endl;
	cout << "IRs is " << Int_adder_rs << " exi  " << Int_adder_ex << "  fui  " << Int_adder_fu << endl;
	cout << "rs f " << FP_adder_rs << " ex f " << FP_adder_ex << "  fu f " << FP_adder_fu << endl;
	cout << "rs fm " << FP_Mul_rs << " ex fm " << FP_Mul_ex << "  fu fm  " << FP_Mul_fu << endl;
	cout << "rs LD " << LdSd_rs << " ex ld " << LdSd_ex << "  Ld mem  " << LdSd_mem << " fu ld  " << LdSd_fu << endl;
	cout << " Rob entries  " << ROB_entries << endl;
	for (int i = 0; i < register_var.size(); i++) // just to check what is in reg_var now
		cout << register_var[i] << "\n";
	for (int i = 0; i < memory_var.size(); i++) // just to check what is in mem_var now
		cout << memory_var[i] << "\n";
	for (int i = 0; i < instructions.size(); i++)
		cout << instructions[i] << "\n"; */

		//--------------------------------------------------------------------------------------------------
		//test to check function is working properly
		/*Inst_content=ReadInstruction(instructions[1]);
		cout << "opcode " << Inst_content[0] << endl;
		cout << "dest " << Inst_content[1] << endl;
		cout << "src1 " << Inst_content[2] << endl;
		cout << "src2 " << Inst_content[3] << endl; */


		/*for (int i = 0; i < instructions.size(); i++)
			cout << instructions[i] << "\n";
		for (int i = 0; i < memory_var.size(); i++) // just to check what is in mem_var now
					cout << memory_var[i] << "\n"; */
					/*for (int i = 0; i < line1.size(); i++)
						cout << line1[i] << "\n"; */



	infile.close();
}