#include <iostream>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <map>

#include <boost/program_options.hpp>

#include "options.hpp"

using namespace std;

namespace pcodedump {

	bool showText;
	bool showLinkage;
	bool listProcs;
	bool addressOrder;
	bool disasmProcs;
	string filename;
	cpu_t cpu;

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
				("address", bool_switch(&addressOrder), "Display code file structures orderd by address")
				("text", bool_switch(&showText), "Display interface text")
				("procs", bool_switch(&listProcs), "Display segment procedures")
				("disasm", bool_switch(&disasmProcs), "Display code disassembly (implies procs)")
				("cpu", value<cpu_t>(&cpu)->default_value(cpu_t::_6502),
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
