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

#include "textio.hpp"

#include <iostream>
#include <iomanip>

using namespace std;

namespace pcodedump {

	void
		line_hexdump(uint8_t * start, uint8_t * finish) {
		for (uint8_t * current = start; current != finish; ++current) {
			wcout << L" " << setw(2) << *current;
		}
		for (int count = 0; count < 16 - distance(start, finish); ++count) {
			wcout << L"   ";
		}
	}

	void
		line_chardump(uint8_t * start, uint8_t * finish) {
		for (uint8_t * current = start; current != finish; ++current) {
			if (32 <= *current && *current <= 0x7e) {
				wcout << char(*current);
			} else {
				wcout << '.';
			}
		}
	}

	void
		hexdump(std::wstring leader, uint8_t * start, uint8_t * finish) {
		FmtSentry<wostream::char_type> sentry{ wcout };
		wcout << hex << nouppercase << right << setfill(L'0');
		unsigned int address = 0;
		uint8_t * current = start;
		while (current != finish) {
			uint8_t * next = distance(current, finish) >= 16 ? current + 16 : finish;
			wcout << leader << setw(4) << address << L":";
			line_hexdump(current, next);
			wcout << L"    ";
			line_chardump(current, next);
			current = next;
			address += 16;
			wcout << endl;
		}
	}

	void
		line_hexdump(buff_t::iterator start, buff_t::iterator finish) {
		for (buff_t::iterator current = start; current != finish; ++current) {
			wcout << L" " << setw(2) << *current;
		}
		for (int count = 0; count < 16 - distance(start, finish); ++count) {
			wcout << L"   ";
		}
	}

	void
		line_chardump(buff_t::iterator start, buff_t::iterator finish) {
		for (buff_t::iterator current = start; current != finish; ++current) {
			if (32 <= *current && *current <= 0x7e) {
				wcout << char(*current);
			} else {
				wcout << '.';
			}
		}
		for (int count = 0; count < 16 - distance(start, finish); ++count) {
			wcout << ' ';
		}
	}

	void
		hexdump(std::wstring leader, buff_t::iterator start, buff_t::iterator finish) {
		FmtSentry<wostream::char_type> sentry{ wcout };
		wcout << hex << nouppercase << setfill(L'0');
		unsigned int address = 0;
		buff_t::iterator current = start;
		while (current != finish) {
			buff_t::iterator next = distance(current, finish) >= 16 ? current + 16 : finish;
			wcout << leader << setw(4) << address << L":";
			line_hexdump(current, next);
			wcout << L"    ";
			line_chardump(current, next);
			current = next;
			address += 16;
			wcout << endl;
		}
	}

	void
		hexdump(buff_t &buffer) {
		hexdump(wstring{ L"" }, begin(buffer), end(buffer));
	}

}
