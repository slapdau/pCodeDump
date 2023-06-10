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

#include "segment.hpp"
#include "native6502.hpp"
#include "pcode.hpp"
#include "types.hpp"
#include "options.hpp"

#include <iterator>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace boost::endian;

namespace pcodedump {

	namespace {

		/* Format a sequence of bytes as a string of space separated 2-digit hex values. */
		wstring toHexString(uint8_t const * begin, uint8_t const * end) {
			wostringstream buff;
			buff << hex << uppercase << setfill(L'0') << right;
			if (begin != end) {
				buff << setw(2) << *begin;
			}
			for (auto current = begin + 1; current != end; ++current) {
				buff << L" " << setw(2) << *current;
			}
			return buff.str();
		}

		class Disassembler final {
		public:
			Disassembler(std::wostream & os, Native6502Procedure const & procedure);

			static void initialiseCpu(cpu_t const cpu);
			std::uint8_t const * decode(std::uint8_t const * current) const;

		private:
			std::uint8_t const * decode_implied(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_immedidate(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_accumulator(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_absolute(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_absoluteindirect(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_absoluteindirectindexed(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_zeropage(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_zeropageindirect(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_absoluteindexedx(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_absoluteindexedy(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_zeropageindexedx(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_zeropageindexedy(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_relative(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_indexedindirect(std::wstring &opCode, std::uint8_t const * current) const;
			std::uint8_t const * decode_indirectindexed(std::wstring &opCode, std::uint8_t const * current) const;

			using decode_function_t = std::uint8_t const * (Disassembler::*)(std::wstring&, std::uint8_t const *) const;
			//using decode_binding_t = decltype(bind(declval<decode_function_t>(), _1, declval<wstring &&>(), _2));
			using decode_binding_t = function<std::uint8_t const *(Disassembler const *, std::uint8_t const *)>;

			static std::vector<decode_binding_t> dispatch;
			static std::map<int, decode_binding_t> dispatch_65c02;

			std::wostream & os;
			Native6502Procedure const & procedure;
		};

		Disassembler::Disassembler(std::wostream & os, Native6502Procedure const & procedure) :
			os{ os }, procedure{ procedure }
		{}

		std::uint8_t const * Disassembler::decode_implied(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 1);
			os << opCode << endl;
			return current + 1;
		}

		std::uint8_t const * Disassembler::decode_immedidate(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" #$" << hex << setfill(L'0') << right << setw(2) << *value << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_accumulator(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 1);
			os << opCode << L" A" << endl;
			return current + 1;
		}

		std::uint8_t const * Disassembler::decode_absolute(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
			os << opCode << L" " << procedure.formatAbsoluteAddress(current + 1) << endl;
			return current + 3;
		}

		std::uint8_t const * Disassembler::decode_absoluteindirect(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
			os << opCode << L" (" << procedure.formatAbsoluteAddress(current + 1) << L")" << endl;
			return current + 3;
		}

		std::uint8_t const * Disassembler::decode_absoluteindirectindexed(std::wstring &opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
			os << opCode << L" (" << procedure.formatAbsoluteAddress(current + 1) << L",X)" << endl;
			return current + 3;
		}

		std::uint8_t const * Disassembler::decode_zeropage(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" $" << hex << setfill(L'0') << right << setw(2) << *value << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_zeropageindirect(std::wstring &opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" ($" << hex << setfill(L'0') << right << setw(2) << *value << L")" << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_absoluteindexedx(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
			os << opCode << L" " << procedure.formatAbsoluteAddress(current + 1) << L",X" << endl;
			return current + 3;
		}

		std::uint8_t const * Disassembler::decode_absoluteindexedy(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
			os << opCode << L" " << procedure.formatAbsoluteAddress(current + 1) << L",Y" << endl;
			return current + 3;
		}

		std::uint8_t const * Disassembler::decode_zeropageindexedx(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" $" << hex << setfill(L'0') << right << setw(2) << *value << L",X" << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_zeropageindexedy(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" $" << hex << setfill(L'0') << right << setw(2) << *value << L",Y" << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_relative(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_int8_t const *>(current + 1);
			os << opCode << L" $" << hex << setfill(L'0') << right << setw(4) << distance(procedure.getProcBegin(), current + 2 + *value) << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_indexedindirect(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" ($" << hex << setfill(L'0') << right << setw(2) << *value << L",X)" << endl;
			return current + 2;
		}

		std::uint8_t const * Disassembler::decode_indirectindexed(std::wstring & opCode, std::uint8_t const * current) const {
			os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
			auto value = reinterpret_cast<little_uint8_t const *>(current + 1);
			os << opCode << L" ($" << hex << setfill(L'0') << right << setw(2) << *value << L"),Y" << endl;
			return current + 2;
		}

		/* Dispatch for opcodes.  For each opcode, the table contains the mneumonic and a pointer
		   the the correct method to decode the address mode.

		   This vector is initialised with 6502 instructions only. Later, based on program options,
		   it might be patched with opcodes for other processors. */
		std::vector<Disassembler::decode_binding_t> Disassembler::dispatch = {
			// 0x00
			bind(&Disassembler::decode_implied, _1, wstring{ L"BRK" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"ASL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"PHP" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_accumulator, _1, wstring{ L"ASL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"ASL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x10
			bind(&Disassembler::decode_relative, _1, wstring{ L"BPL" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"ASL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"CLC" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"ORA" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"ASL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x20
			bind(&Disassembler::decode_absolute, _1, wstring{ L"JSR" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"BIT" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"ROL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"PLP" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_accumulator, _1, wstring{ L"ROL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"BIT" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"ROL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x30
			bind(&Disassembler::decode_relative, _1, wstring{ L"BMI" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"ROL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"SEC" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"AND" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"ROL" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x40
			bind(&Disassembler::decode_implied, _1, wstring{ L"RTI" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"LSR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"PHA" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_accumulator, _1, wstring{ L"LSR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"JMP" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"LSR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x50
			bind(&Disassembler::decode_relative, _1, wstring{ L"BVC" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"LSR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"CLI" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"EOR" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"LSR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x60
			bind(&Disassembler::decode_implied, _1, wstring{ L"RTS" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"ROR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"PLA" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_accumulator, _1, wstring{ L"ROR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindirect, _1, wstring{ L"JMP" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"ROR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x70
			bind(&Disassembler::decode_relative, _1, wstring{ L"BVS" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"ROR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"SEI" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"ADC" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"ROR" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x80
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"STY" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"STX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"DEY" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"TXA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"STY" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"STX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0x90
			bind(&Disassembler::decode_relative, _1, wstring{ L"BCC" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"STY" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_zeropageindexedy, _1, wstring{ L"STX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"TYA" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"TXS" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"STA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0xA0
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"LDY" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"LDX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"LDY" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"LDX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"TAY" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"TAX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"LDY" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"LDX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0xB0
			bind(&Disassembler::decode_relative, _1, wstring{ L"BCS" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"LDY" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_zeropageindexedy, _1, wstring{ L"LDX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"CLV" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"TSX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"LDY" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"LDA" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"LDX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0xC0
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"CPY" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"CPY" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"DEC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"INY" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"DEX" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"CPY" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"DEC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0xD0
			bind(&Disassembler::decode_relative, _1, wstring{ L"BNE" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"DEC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"CLD" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"CMP" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"DEC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0xE0
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"CPX" }, _2),
			bind(&Disassembler::decode_indexedindirect, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"CPX" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_zeropage, _1, wstring{ L"INC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"INX" }, _2),
			bind(&Disassembler::decode_immedidate, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"NOP" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"CPX" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_absolute, _1, wstring{ L"INC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			// 0xF0
			bind(&Disassembler::decode_relative, _1, wstring{ L"BEQ" }, _2),
			bind(&Disassembler::decode_indirectindexed, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"INC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"SED" }, _2),
			bind(&Disassembler::decode_absoluteindexedy, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"SBC" }, _2),
			bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"INC" }, _2),
			bind(&Disassembler::decode_implied, _1, wstring{ L"???" }, _2),
		};

		/* Opcode patches for the dispatch table if the 65c02 is chosen. */
		std::map<int, Disassembler::decode_binding_t> Disassembler::dispatch_65c02 = {
			{0x04,bind(&Disassembler::decode_zeropage, _1, wstring{ L"TSB" }, _2)},
			{0x0C,bind(&Disassembler::decode_absolute, _1, wstring{ L"TSB" }, _2)},
			{0x12,bind(&Disassembler::decode_zeropageindirect, _1, wstring{ L"ORA" }, _2)},
			{0x14,bind(&Disassembler::decode_zeropage, _1, wstring{ L"TRB" }, _2)},
			{0x1A,bind(&Disassembler::decode_accumulator, _1, wstring{ L"INC" }, _2)},
			{0x1C,bind(&Disassembler::decode_absolute, _1, wstring{ L"TRB" }, _2)},
			{0x32,bind(&Disassembler::decode_zeropage, _1, wstring{ L"AND" }, _2)},
			{0x34,bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"BIT" }, _2)},
			{0x3A,bind(&Disassembler::decode_accumulator, _1, wstring{ L"DEC" }, _2)},
			{0x3C,bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"BIT" }, _2)},
			{0x52,bind(&Disassembler::decode_zeropage, _1, wstring{ L"EOR" }, _2)},
			{0x5A,bind(&Disassembler::decode_implied, _1, wstring{ L"PHY" }, _2)},
			{0x64,bind(&Disassembler::decode_zeropage, _1, wstring{ L"STZ" }, _2)},
			{0x72,bind(&Disassembler::decode_zeropage, _1, wstring{ L"ADC" }, _2)},
			{0x74,bind(&Disassembler::decode_zeropageindexedx, _1, wstring{ L"STZ" }, _2)},
			{0x7A,bind(&Disassembler::decode_implied, _1, wstring{ L"PLY" }, _2)},
			{0x7C,bind(&Disassembler::decode_absoluteindirectindexed, _1, wstring{ L"JMP" }, _2)},
			{0x80,bind(&Disassembler::decode_relative, _1, wstring{ L"BRA" }, _2)},
			{0x89,bind(&Disassembler::decode_immedidate, _1, wstring{ L"BIT" }, _2)},
			{0x92,bind(&Disassembler::decode_zeropage, _1, wstring{ L"STA" }, _2)},
			{0x9C,bind(&Disassembler::decode_absolute, _1, wstring{ L"STZ" }, _2)},
			{0x9E,bind(&Disassembler::decode_absoluteindexedx, _1, wstring{ L"STZ" }, _2)},
			{0xB2,bind(&Disassembler::decode_zeropage, _1, wstring{ L"LDA" }, _2)},
			{0xD2,bind(&Disassembler::decode_zeropage, _1, wstring{ L"CMP" }, _2)},
			{0xDA,bind(&Disassembler::decode_implied, _1, wstring{ L"PHX" }, _2)},
			{0xF2,bind(&Disassembler::decode_zeropage, _1, wstring{ L"SBC" }, _2)},
			{0xFA,bind(&Disassembler::decode_implied, _1, wstring{ L"PLX" }, _2)},
		};

		void Disassembler::initialiseCpu(cpu_t const cpu) {
			if (cpu == cpu_t::_65c02) {
				for (auto[instruction, patch] : dispatch_65c02) {
					dispatch[instruction] = patch;
				}
			}
		}

		std::uint8_t const * Disassembler::decode(std::uint8_t const * current) const {
			auto opcode = *current;
			return dispatch[opcode](this, current);
		}

	}

	/* Read one of the 4 6502 procedure relocation tables. Return a pointer to the start of the table. */
	uint8_t const * Native6502Procedure::readRelocations(Relocations &table, uint8_t const * rawTable) {
		auto current = rawTable;
		current -= sizeof(little_uint16_t);
		int total = *reinterpret_cast<little_uint16_t const *>(current);
		for (int count = 0; count != total; ++count) {
			current -= sizeof(little_uint16_t);
			table.push_back(derefSelfPtr(current));
		}
		return current;
	}

	namespace {

		template <typename T>
		bool contains(vector<T> vect, T value) {
			return find(cbegin(vect), cend(vect), value) != cend(vect);
		}

	}

	/* Format a 16-bit absolute address for display.  The address is embedded in a 6502 instruction.
	   If the address is referred to by either the segment or interpreter relocation tables, indicate
	   this in the formatting. */
	wstring Native6502Procedure::formatAbsoluteAddress(uint8_t const * address) const {
		auto value = *reinterpret_cast<little_uint16_t const *>(address);
		wostringstream result;
		if (pcodedump::contains(segRelocations, address)) {
			uint8_t const * target = codePart.begin() + value;
			Procedure const * targetProc = codePart.findProcedure(target);
			if (targetProc) {
				value = static_cast<int>(target - targetProc->getProcBegin());
				result << L".proc#" << dec << targetProc->getProcedureNumber() << "+";
			} else {
				result << L".seg+";
			}
		} else if (pcodedump::contains(interpRelocations, address)) {
			result << L".interp+";
		} else if (pcodedump::contains(baseRelocations, address)) {
			if (attributeTable->relocationSeg != 0) {
				result << L".seg#" << dec << attributeTable->relocationSeg << "+";
			} else {
				result << L".base+";
			}
		} else if (pcodedump::contains(procRelocations, address)) {
			result << L".proc+";
		}
		result << L"$" << uppercase << hex << setfill(L'0') << right << setw(4) << value;
		return result.str();
	}

	Native6502Procedure::Native6502Procedure(CodePart & codePart, int procedureNumber, Range<std::uint8_t const> data) :
		base(codePart, procedureNumber, data),
		attributeTable{ reinterpret_cast<AttributeTable const *>(data.end() - sizeof(AttributeTable)) }
	{
		this->procEnd = data.end() - sizeof(AttributeTable);
		for (auto table : { &baseRelocations, &segRelocations, &procRelocations, &interpRelocations }) {
			procEnd = readRelocations(*table, procEnd);
		}
	}

	   
	/* Check the nominated CPU type, and patch the opcode decoding dispatch table if necessary. */
	void Native6502Procedure::initialiseCpu(cpu_t const & cpu) {
		Disassembler::initialiseCpu(cpu);
	}

	void Native6502Procedure::writeHeader(std::wostream & os) const {
		auto procBegin = data.begin();
		auto procLength = data.end() - data.begin();
		os << "Proc #" << dec << setfill(L' ') << left << setw(4) << procedureNumber << L" (";
		os << hex << setfill(L'0') << right << setw(4) << distance(codePart.begin(), procBegin) << ":" << setw(4) << distance(codePart.begin(), procBegin) + procLength - 1<< ") Native (6502)  ";
		os << endl;
	}

	/* Write a disassembly of the procedure to an output stream. */
	void Native6502Procedure::disassemble(std::wostream & os) const {
		Disassembler disassember{ os, *this};
		uint8_t const * ic = data.begin();
		while (ic && ic < procEnd) {
			printIc(os, ic);
			ic = disassember.decode(ic);
		}
	}

	std::uint8_t const * Native6502Procedure::getEnterIc() const {
		return derefSelfPtr(reinterpret_cast<std::uint8_t const *>(&attributeTable->enterIc));
	}

	/* Write the instruction address, relative to the segment start.  Indicate the procedure entry point. */
	void Native6502Procedure::printIc(std::wostream & os, std::uint8_t const * current) const {
		if (getEnterIc() == current) {
			os << L"  ENTER:" << endl;
		}
		os << L"   ";
		os << hex << setfill(L'0') << right << setw(4) <<  current - this->getProcBegin() << L": ";
	}


}
