/*
   Copyright 2017 Craig McGeachie

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

#include <iostream>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <map>
#include <functional>

#include <boost/program_options.hpp>

#include "options.hpp"
#include "native6502.hpp"

using namespace std;

namespace pcodedump {

	bool showText;
	bool showLinkage;
	bool listProcs;
	bool disasmProcs;
	string filename;

	namespace {
		map<string, cpu_t> string_to_cpu = {
			{"6502", cpu_t::_6502},
			{"65c02", cpu_t::_65c02},
		};

		map<cpu_t, string> cpu_to_string = {
			{cpu_t::_6502, "6502"},
			{cpu_t::_65c02, "65c02"},
			{cpu_t::_65c816, "65c816"},
		};
	}

	istream& operator >> (istream& in, cpu_t & cpu) {
		string token;
		in >> token;
		if (string_to_cpu.count(token)) {
			cpu = string_to_cpu[token];
		} else {
			throw new boost::program_options::invalid_option_value{ "Invalid CPU type" };
		}
		return in;
	}

	ostream & operator << (ostream & out, cpu_t cpu) {
		out << cpu_to_string[cpu];
		return out;
	}

	/* Parse program options and store the values in global values. Return true if the program
	   should then continue processing. */
	bool parseOptions(int argc, char *argv[], char *envp[]) {
		using namespace boost::program_options;

		try {
			bool help;

			options_description opts{ "pcodedump" };
			opts.add_options()
				("help", bool_switch(&help), "Display this message")
				("text", bool_switch(&showText), "Display interface text")
				("procs", bool_switch(&listProcs), "Display segment procedures")
				("disasm", bool_switch(&disasmProcs), "Display code disassembly (implies procs)")
				("cpu", value<cpu_t>()->default_value(cpu_t::_6502)->notifier(Native6502Procedure::initialiseCpu),
					"CPU type for disassembled native code:\n"
					"  6502\n"
					"  65c02")
					("link", bool_switch(&showLinkage), "Display linker information");
			options_description allopts{ "All options" };
			allopts.add_options()
				("input-file", value<string>(&filename), "");
			allopts.add(opts);
			positional_options_description positional{};
			positional.add("input-file", 1);
			variables_map vm;
			store(command_line_parser(argc, argv).options(allopts).positional(positional).run(), vm);
			notify(vm);
			listProcs |= disasmProcs;

			if (help) {
				cout << opts << endl;
			}
			return !help;
		} catch (boost::program_options::error &ex) {
			cout << ex.what() << endl;
			return false;
		}
	}
}
