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

#include "textio.hpp"

#include <iomanip>

using namespace std;

namespace pcodedump {

	void line_hexdump(wostream & out, const uint8_t * start, const uint8_t * finish)
	{
		for (auto current = start; current != finish; ++current) {
			out << L" " << setw(2) << *current;
		}
		for (int count = 0; count < 16 - distance(start, finish); ++count) {
			out << L"   ";
		}
	}

	void line_chardump(wostream & out, uint8_t const * start, uint8_t const * finish) {
		for (auto current = start; current != finish; ++current) {
			if (32 <= *current && *current <= 0x7e) {
				out << char(*current);
			} else {
				out << '.';
			}
		}
	}

	void hexdump(wostream & out, std::wstring const & leader, uint8_t const * start, uint8_t const * finish) {
		FmtSentry<wostream::char_type> sentry{ out };
		out << hex << nouppercase << right << setfill(L'0');
		unsigned int address = 0;
		uint8_t const * current = start;
		while (current != finish) {
			uint8_t const * next = distance(current, finish) >= 16 ? current + 16 : finish;
			out << leader << setw(4) << address << L":";
			line_hexdump(out, current, next);
			out << L"    ";
			line_chardump(out, current, next);
			current = next;
			address += 16;
			out << endl;
		}
	}

	void line_hexdump(wostream & out, buff_t::const_iterator start, buff_t::const_iterator finish) {
		for (buff_t::const_iterator current = start; current != finish; ++current) {
			out << L" " << setw(2) << *current;
		}
		for (int count = 0; count < 16 - distance(start, finish); ++count) {
			out << L"   ";
		}
	}

	void line_chardump(wostream & out, buff_t::const_iterator start, buff_t::const_iterator finish) {
		for (buff_t::const_iterator current = start; current != finish; ++current) {
			if (32 <= *current && *current <= 0x7e) {
				out << char(*current);
			} else {
				out << '.';
			}
		}
		for (int count = 0; count < 16 - distance(start, finish); ++count) {
			out << ' ';
		}
	}

	void hexdump(wostream & out, std::wstring const & leader, buff_t::const_iterator start, buff_t::const_iterator finish) {
		FmtSentry<wostream::char_type> sentry{ out };
		out << hex << nouppercase << setfill(L'0');
		unsigned int address = 0;
		buff_t::const_iterator current = start;
		while (current != finish) {
			buff_t::const_iterator next = distance(current, finish) >= 16 ? current + 16 : finish;
			out << leader << setw(4) << address << L":";
			line_hexdump(out, current, next);
			out << L"    ";
			line_chardump(out, current, next);
			current = next;
			address += 16;
			out << endl;
		}
	}

	void hexdump(wostream & out, buff_t const & buffer) {
		hexdump(out, L"", begin(buffer), end(buffer));
	}

}
