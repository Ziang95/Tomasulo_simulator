target = mips
cpp = g++
cpp_files = $(shell dir /b *.cpp )
head_files = $(shell dir /b *.h)

$(target):$(cpp_files) $(head_files)
	$(cpp) -g -std=c++11 -pthread -o $(target).exe $(cpp_files)