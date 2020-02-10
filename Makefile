target = mips
cpp = g++

ifeq ($(OS), Windows_NT)
cpp_files = $(shell dir /b *.cpp )
$(target):$(cpp_files)
	$(cpp) -g -std=c++11 -pthread -o $(target).exe $(cpp_files)
else
UNAME = $(shell uname -s)
ifeq ($(UNAME), Linux)
cpp_files = $(shell ls *.cpp )
$(target):$(cpp_files)
	$(cpp) -g -std=c++11 -pthread -o $(target) $(cpp_files)
endif
endif