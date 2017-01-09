// PcodeDisassemble.cpp : Defines the entry point for the console application.
//

#include "directory.hpp"
#include "types.hpp"
#include "options.hpp"
#include "native6502.hpp"

#include <iostream>
#include <fstream>
#include <string>


using namespace std;

namespace pcodedump {

#if 0
	void
		readBlock(file_t & file, buff_t & buffer, int blockNumber) {
		file.seekg(blockNumber * BLOCK_SIZE);
		file.read(buffer.data(), BLOCK_SIZE);
	}
#endif

	unique_ptr<buff_t> readFile(wstring filename) {
		using file_t = std::basic_ifstream<std::uint8_t>;
		file_t file{filename, ios_base::in | ios_base::binary | ios_base::ate};
		file.exceptions(ifstream::failbit);
		streamsize size = file.tellg();
		auto buffer = make_unique<buff_t>(static_cast<int>(size));
		file.seekg(0);
		file.read(buffer->data(), static_cast<unsigned int>(size));
		file.close();
		return buffer;
	}

}

int
wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {	
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
