#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

class config
{
	// this class is used to read input.txt and give back all the info needed to start.

private:
	vector<string> Inst_content;
	int number_of_lines = 0;
	vector<string> input;
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
	
public:
	vector<string> instructions;
	vector<string> memory_var;
	vector<string> register_var;
	void ReadInput();// read input file and remove extra spaces and commas
	int convertStringToInt(string& input); // Convert string to int
	double convertStringToFloat(string& input); //convert string to float
	vector<string> ReadInstruction(string str); //geting opccode, dest, src1, src2
	void SetVariables();//read data line by line and get unique info from each line. input format is constant just reg value and memory will increase
	int getROB_LEN() const {
		return ROB_entries;
}
	int getINTADDR_RS_NUM() const {
		return Int_adder_rs;
	}
	int getINTADDR_EX_TIME() const {
		return Int_adder_ex;
	}
	int getINTADDR_FU_NUM() const {
		return Int_adder_fu;
	}
	int getFPADDR_RS_NUM() const {
		return FP_adder_rs;
	}
	int getFPADDR_EX_TIME() const {
		return FP_adder_ex;
	}
	int getFPADDR_FU_NUM() const {
		return FP_adder_fu;
	}
	int getFPMTPLR_RS_NUM() const {
		return FP_Mul_rs;
	}
	int getFPMTPLR_EX_TIME() const {
		return FP_Mul_ex;
	}
	int getFPMTPLR_FU_NUM() const {
		return FP_Mul_fu;
	}
	int getLD_STR_RS_NUM() const {
		return LdSd_rs;
	}
	int getLD_STR_EX_TIME() const {
		return LdSd_ex;
	}
	int getLD_STR_MEM_TIME() const {
		return LdSd_mem;
	}
	int getLD_STR_FU_NUM() const {
		return LdSd_fu;
	}

};

#endif
