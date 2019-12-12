target = mips
cpp = g++
cpp_files = $(shell dir /b *.cpp )

$(target):$(cpp_files)
	$(cpp) -g -std=c++11 -pthread -o $(target).exe $(cpp_files)