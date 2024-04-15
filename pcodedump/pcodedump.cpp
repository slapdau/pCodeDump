/*
   Copyright 2017-2024 Craig McGeachie

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "pcodefile.hpp"
#include "types.hpp"
#include "options.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <system_error>

using namespace std;

namespace {
	using pcodedump::buff_t;

	unique_ptr<buff_t const> readFile(filesystem::path filename) {
		using file_t = std::ifstream;
	    using iterator_t = istreambuf_iterator<file_t::char_type>;

		if (!filesystem::is_regular_file(filename)) {
			throw runtime_error(string("File not found: ") + filename.string());
		}

	    file_t file(filename, ios_base::binary);
		file.exceptions(ifstream::failbit);
		auto buffer = make_unique<buff_t>();
		buffer->reserve(filesystem::file_size(filename));
		buffer->assign(iterator_t(file), iterator_t());
		file.close();
		return buffer;
	}
}

int
main(int argc, char *argv[]) {
	using namespace pcodedump;

	try {
		if (parseOptions(argc, argv)) {
			auto buffer = readFile(filename);
			PcodeFile file{*buffer};
			wcout << file;
		}
		return 0;
	} catch (system_error &ex) {
		cout << ex.what() << ": " << ex.code().message() << endl;
		return 1;
	} catch (exception& ex) {
		cout << ex.what() << endl;
		return 1;
	}
}
