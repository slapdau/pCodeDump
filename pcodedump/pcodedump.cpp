// PcodeDisassemble.cpp : Defines the entry point for the console application.
//

#include "directory.hpp"
#include "types.hpp"
#include "options.hpp"
#include "native6502.hpp"

#include <iostream>
#include <fstream>
#include <string>



namespace pcodedump {

    using namespace std;

#if 0
	void
		readBlock(file_t & file, buff_t & buffer, int blockNumber) {
		file.seekg(blockNumber * BLOCK_SIZE);
		file.read(buffer.data(), BLOCK_SIZE);
	}
#endif

	unique_ptr<buff_t> readFile(string filename) {
		using file_t = std::ifstream;
	    using iterator_t = istreambuf_iterator<file_t::char_type>;

	    file_t file(filename, ios_base::binary);
		file.exceptions(ifstream::failbit);
		auto buffer = make_unique<buff_t>();
		buffer->assign(iterator_t(file), iterator_t());
		file.close();
		return buffer;
	}

}

int
main(int argc, char *argv[], char *envp[]) {
	using namespace pcodedump;

	try {
		if (parseOptions(argc, argv, envp)) {
			Native6502Procedure::initialiseCpu();
			unique_ptr<buff_t> buffer = readFile(filename);
			SegmentDirectory directory{*buffer};
			wcout << directory;
		}
		return 0;
	} catch (ios::failure &ex) {
		cout << ex.code().message() << endl;
		return 1;
	}
}
