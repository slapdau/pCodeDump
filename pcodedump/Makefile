CXXFLAGS=-std=c++14 -O0 -g -I /home/craig/Development/boost_1_62_0

pcodedump: basecode.o directory.o linkage.o native6502.o options.o pcodedump.o pcode.o textio.o text.o
	$(CXX) -L /home/craig/Development/boost_1_62_0/stage/lib -l boost_program_options-gcc53-mt-1_62 -o $@ $^
	
clean:
	rm -f pcodedump *.o
