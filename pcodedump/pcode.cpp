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

#include "pcode.hpp"
#include "segment.hpp"
#include "types.hpp"
#include "textio.hpp"
#include "options.hpp"
#include "linkage.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace boost::endian;

namespace {

	template <typename T>
	inline T getNext(std::uint8_t const *& address) {
		T val = *reinterpret_cast<T const *>(address);
		address += sizeof(T);
		return val;
	}

	inline int16_t getNextBig(std::uint8_t const *& address) {
		int16_t val = getNext<uint8_t>(address);
		if (val & 0x80) {
			val = ((val & 0x7f) << 8) + getNext<uint8_t>(address);
		}
		return val;
	}

}

namespace pcodedump {

	class PcodeProcedure::Disassembler final {
	public:
		Disassembler(std::wostream& os, PcodeProcedure const& procedure, linkref_map_t& linkage);

		std::uint8_t const* decode(std::uint8_t const* current) const;

	private:
		inline intptr_t getNextJumpAddress(std::uint8_t const *& address) const;
		inline intptr_t getNextCaseAddress(std::uint8_t const *& address) const;

		std::uint8_t const* decode_implied(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_unsignedByte(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_big(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_intermediate(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_extended(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_word(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_wordBlock(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_stringConstant(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_packedConstant(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_jump(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_return(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_doubleByte(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_case(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_callStandardProc(std::wstring& opCode, std::uint8_t const* current) const;
		std::uint8_t const* decode_compare(std::wstring& opCode, std::uint8_t const* current) const;

		using decode_function_t = std::uint8_t const* (Disassembler::*)(std::wstring&, std::uint8_t const*) const;
		// using decode_binding_t = decltype(bind(declval<decode_function_t>(), _1, declval<wstring &&>(), _2));
		using decode_binding_t = function<std::uint8_t const* (Disassembler const*, std::uint8_t const*)>;

		static std::vector<decode_binding_t> dispatch;

		std::wostream& os;
		PcodeProcedure const& procedure;
		linkref_map_t& linkage;
	};

	PcodeProcedure::Disassembler::Disassembler(std::wostream& os, PcodeProcedure const& procedure, linkref_map_t& linkage) :
		os{ os }, procedure{ procedure }, linkage{ linkage }
	{}

	inline intptr_t PcodeProcedure::Disassembler::getNextJumpAddress(std::uint8_t const *& address) const {
		auto offset = getNext<int8_t>(address);
		if (offset >= 0) {
			return (address + offset) - procedure.getProcBegin();
		} else {
			return derefSelfPtr(procedure.jtab(offset)) - procedure.getProcBegin();
		}

	}

	inline intptr_t PcodeProcedure::Disassembler::getNextCaseAddress(std::uint8_t const *& address) const
	{
		auto result = derefSelfPtr(address) - procedure.getProcBegin();
		address += 2;
		return result;
	}

	uint8_t const* PcodeProcedure::Disassembler::decode_implied(wstring& opCode, uint8_t const* current) const {
		os << opCode << endl;
		return current;
	}

	/* ub */
	uint8_t const* PcodeProcedure::Disassembler::decode_unsignedByte(wstring& opCode, uint8_t const* current)  const {
		os << setfill(L' ') << left << setw(9) << opCode << dec << getNext<uint8_t>(current) << endl;
		return current;
	}

	/* b */
	uint8_t const * PcodeProcedure::Disassembler::decode_big(wstring &opCode, uint8_t const * current)  const {
		if (linkage.count(current)) {
			os << setfill(L' ') << left << setw(9) << opCode << L"<" << linkage[current]->getName() << L">" << endl;
			current += 2;
		} else {
			os << setfill(L' ') << left << setw(9) << opCode << dec << getNextBig(current) << endl;
		}
		return current;
	}

	/* db, b */
	uint8_t const* PcodeProcedure::Disassembler::decode_intermediate(wstring& opCode, uint8_t const* current) const {
		auto linkCount = getNext<uint8_t>(current);
		auto offset = getNextBig(current);
		os << setfill(L' ') << left << setw(9) << opCode << dec << linkCount << L", " << offset << endl;
		return current;
	}

	/* ub, b */
	uint8_t const* PcodeProcedure::Disassembler::decode_extended(wstring& opCode, uint8_t const* current)  const {
		auto dataSegment = getNext<uint8_t>(current);
		auto offset = getNextBig(current);
		os << setfill(L' ') << left << setw(9) << opCode << dec << dataSegment << L", " << offset << endl;
		return current;
	}

	/* w */
	uint8_t const* PcodeProcedure::Disassembler::decode_word(wstring& opCode, uint8_t const* current)  const {
		os << setfill(L' ') << left << setw(9) << opCode << dec << getNext<little_int16_t>(current) << endl;
		return current;
	}

	float convertToReal(uint8_t const* buff) {
		auto words = reinterpret_cast<little_int16_t const*>(buff);
		uint16_t buf[2] = {};
		buf[1] = words[0];
		buf[0] = words[1];
		return *reinterpret_cast<float*>(buf);
	}

	/* ub, word aligned block of words */
	uint8_t const* PcodeProcedure::Disassembler::decode_wordBlock(wstring& opCode, uint8_t const* current)  const {
		auto total = getNext<uint8_t>(current);
		current = procedure.align<little_int16_t>(current);
		os << setfill(L' ') << left << setw(9) << opCode << dec << setw(9) << total;
		if (total == 2) {
			os << L"; As a real value: " << convertToReal(current);
		}
		os << endl;
		for (int count = 0; count != total; ++count) {
			auto value = getNext<little_int16_t>(current);
			os
				<< setfill(L' ') << setw(18) << L""
				<< setfill(L' ') << left << dec << setw(9) << value
				<< L"; $" << setfill(L'0') << hex << setw(4) << value
				<< endl;
		}
		return current;
	}

	/* ub, <chars> */
	uint8_t const* PcodeProcedure::Disassembler::decode_stringConstant(wstring& opCode, uint8_t const* current) const {
		auto total = getNext<uint8_t>(current);
		os << setfill(L' ') << left << setw(9) << opCode << dec << total << endl;
		uint8_t const* finish = current + total;
		FmtSentry<wostream::char_type> sentry{ wcout };
		while (current != finish) {
			uint8_t const* next = distance(current, finish) >= 80 ? current + 80 : finish;
			wcout << L"                  ";
			line_chardump(wcout, current, next);
			current = next;
			wcout << endl;
		}
		return finish;
	}

	/* ub, <bytes> */
	uint8_t const* PcodeProcedure::Disassembler::decode_packedConstant(wstring& opCode, uint8_t const* current) const {
		auto count = getNext<uint8_t>(current);
		os << setfill(L' ') << left << setw(9) << opCode << dec << count << endl;
		hexdump(wcout, L"                  " , current, current + count);
		current += count;
		return current;
	}

	/* sb */
	uint8_t const* PcodeProcedure::Disassembler::decode_jump(wstring& opCode, uint8_t const* current) const {
		os << setfill(L' ') << left << setw(9) << opCode << L"(" << hex << setfill(L'0') << right << setw(4) << getNextJumpAddress(current) << L")" << endl;
		return current;
	}

	/* db */
	uint8_t const* PcodeProcedure::Disassembler::decode_return(wstring& opCode, uint8_t const* current) const {
		os << opCode << endl;
		return nullptr;
	}

	/* ub, ub */
	uint8_t const* PcodeProcedure::Disassembler::decode_doubleByte(wstring& opCode, uint8_t const* current) const {
		if (linkage.count(current)) {
			auto segName = linkage[current]->getName();
			current += 1;
			auto value_2 = getNext<uint8_t>(current);
			os << setfill(L' ') << left << setw(9) << opCode << dec << L"<" << segName << L">, " << value_2 << endl;
		} else {
			auto value_1 = getNext<uint8_t>(current);
			auto value_2 = getNext<uint8_t>(current);
			os << setfill(L' ') << left << setw(9) << opCode << dec << value_1 << L", " << value_2 << endl;
		}
		return current;
	}

	/* word aligned -> idx_min, idx_max, (ujp sb), table */
	uint8_t const* PcodeProcedure::Disassembler::decode_case(wstring& opCode, uint8_t const* current)  const {
		current = procedure.align<little_int16_t>(current);
		auto min = getNext<little_int16_t>(current);
		auto max = getNext<little_int16_t>(current);
		current++; // Skip the UJP opcode
		os << setfill(L' ') << left << setw(9) << opCode << dec << min << ", " << max << " (" << hex << setfill(L'0') << right << setw(4) << getNextJumpAddress(current) << ")" << endl;
		for (int count = min; count != max + 1; ++count) {
			os << setfill(L' ') << setw(18) << L"" << L"(" << hex << setfill(L'0') << right << setw(4) << getNextCaseAddress(current) << L")" << endl;
		}
		return current;
	}

	map<int, wstring> standardProcs = {
		{ 0, L"iocheck" },
		{ 1, L"new" },
		{ 2, L"moveleft" },
		{ 3, L"moveright" },
		{ 4, L"exit" },
		{ 5, L"unitread" },
		{ 6, L"unitwrite" },
		{ 7, L"idsearch" },
		{ 8, L"treesearch" },
		{ 9, L"time" },
		{ 10, L"fillchar" },
		{ 11, L"scan" },
		{ 12, L"unitstatus" },
		{ 21, L"getseg" },
		{ 22, L"relseg" },
		{ 23, L"trunc" },
		{ 24, L"round" },
#if 1
		// These are standard UCSD p-code standard procedures that are not implemented
		// in Apple Pascal, which instead provides them in the transcendental intrinsic unit.
		// Left in because it doesn't seem to hurt.
		{ 25, L"sine" },
		{ 26, L"cos" },
		{ 27, L"log" },
		{ 28, L"atan" },
		{ 29, L"ln" },
		{ 30, L"exp" },
		{ 31, L"sqrt" },
#endif
		{ 32, L"mark" },
		{ 33, L"release" },
		{ 34, L"ioresult" },
		{ 35, L"unitbusy" },
		{ 36, L"pwroften" },
		{ 37, L"unitwait" },
		{ 38, L"unitclear" },
		{ 39, L"halt" },
		{ 40, L"memavail" },
	};

	/* CSP ub */
	uint8_t const* PcodeProcedure::Disassembler::decode_callStandardProc(wstring& opCode, uint8_t const* current)  const {
		int standardProcNumber = *current++;
		os << setfill(L' ') << left << setw(9) << opCode << dec << setw(6) << standardProcNumber;
		if (standardProcs.count(standardProcNumber)) {
			os << L"; " << standardProcs[standardProcNumber];
		}
		os << endl;
		return current;
	}

	/* 2-reals, 4-strings, 6-booleans, 8-sets, 10-byte arrays, 12-words. 10 and 12 have b as well */
	uint8_t const* PcodeProcedure::Disassembler::decode_compare(wstring& opCode, uint8_t const* current)  const {
		os << opCode << L" ";
		switch (*current++) {
		case 2:
			os << L"REAL" << endl;
			break;
		case 4:
			os << L"STR" << endl;
			break;
		case 6:
			os << L"BOOL" << endl;
			break;
		case 8:
			os << L"SET" << endl;
			break;
		case 10: {
			int byteCount = *current++;
			if (byteCount & 0x80) {
				byteCount = ((byteCount & 0x7f) << 8) + *current++;
			}
			os << L"BYTE " << dec << byteCount << endl;
			break;
		}
		case 12: {
			int byteCount = *current++;
			if (byteCount & 0x80) {
				byteCount = ((byteCount & 0x7f) << 8) + *current++;
			}
			os << L"WORD " << dec << byteCount << endl;
			break;
		}
		default:
			os << L"<undefined> (0x" << hex << setfill(L'0') << right << setw(2) << *(current - 1) << L")" << endl;
			break;
		}
		return current;
	}

	vector<PcodeProcedure::Disassembler::decode_binding_t> PcodeProcedure::Disassembler::dispatch = {
		bind(&decode_implied, _1, wstring{ L"SDLC_0" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_1" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_2" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_3" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_4" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_5" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_6" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_7" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_8" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_9" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_10" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_11" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_12" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_13" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_14" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_15" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_16" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_17" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_18" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_19" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_20" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_21" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_22" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_23" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_24" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_25" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_26" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_27" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_28" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_29" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_30" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_31" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_32" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_33" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_34" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_35" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_36" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_37" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_38" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_39" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_40" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_41" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_42" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_43" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_44" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_45" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_46" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_47" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_48" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_49" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_50" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_51" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_52" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_53" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_54" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_55" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_56" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_57" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_58" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_59" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_60" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_61" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_62" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_63" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_64" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_65" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_66" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_67" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_68" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_69" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_70" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_71" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_72" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_73" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_74" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_75" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_76" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_77" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_78" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_79" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_80" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_81" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_82" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_83" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_84" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_85" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_86" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_87" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_88" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_89" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_90" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_91" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_92" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_93" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_94" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_95" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_96" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_97" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_98" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_99" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_100" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_101" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_102" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_103" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_104" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_105" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_106" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_107" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_108" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_109" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_110" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_111" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_112" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_113" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_114" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_115" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_116" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_117" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_118" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_119" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_120" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_121" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_122" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_123" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_124" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_125" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_126" }, _2),
		bind(&decode_implied, _1, wstring{ L"SDLC_127" }, _2),
		bind(&decode_implied, _1, wstring{ L"ABI" }, _2),
		bind(&decode_implied, _1, wstring{ L"ABR" }, _2),
		bind(&decode_implied, _1, wstring{ L"ADI" }, _2),
		bind(&decode_implied, _1, wstring{ L"ADR" }, _2),
		bind(&decode_implied, _1, wstring{ L"LAND" }, _2),
		bind(&decode_implied, _1, wstring{ L"DIF" }, _2),
		bind(&decode_implied, _1, wstring{ L"DVI" }, _2),
		bind(&decode_implied, _1, wstring{ L"DVR" }, _2),
		bind(&decode_implied, _1, wstring{ L"CHK" }, _2),
		bind(&decode_implied, _1, wstring{ L"FLO" }, _2),
		bind(&decode_implied, _1, wstring{ L"FLT" }, _2),
		bind(&decode_implied, _1, wstring{ L"INN" }, _2),
		bind(&decode_implied, _1, wstring{ L"INT" }, _2),
		bind(&decode_implied, _1, wstring{ L"LOR" }, _2),
		bind(&decode_implied, _1, wstring{ L"MODI" }, _2),
		bind(&decode_implied, _1, wstring{ L"MPI" }, _2),
		bind(&decode_implied, _1, wstring{ L"MPR" }, _2),
		bind(&decode_implied, _1, wstring{ L"NGI" }, _2),
		bind(&decode_implied, _1, wstring{ L"NGR" }, _2),
		bind(&decode_implied, _1, wstring{ L"LNOT" }, _2),
		bind(&decode_implied, _1, wstring{ L"SRS" }, _2),
		bind(&decode_implied, _1, wstring{ L"SBI" }, _2),
		bind(&decode_implied, _1, wstring{ L"SBR" }, _2),
		bind(&decode_implied, _1, wstring{ L"SGS" }, _2),
		bind(&decode_implied, _1, wstring{ L"SQI" }, _2),
		bind(&decode_implied, _1, wstring{ L"SQR" }, _2),
		bind(&decode_implied, _1, wstring{ L"STO" }, _2),
		bind(&decode_implied, _1, wstring{ L"IXS" }, _2),
		bind(&decode_implied, _1, wstring{ L"UNI" }, _2),
		bind(&decode_extended, _1, wstring{ L"LDE" }, _2),
		bind(&decode_callStandardProc, _1, wstring{ L"CSP" }, _2),
		bind(&decode_implied, _1, wstring{ L"LDCN" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"ADJ" }, _2),
		bind(&decode_jump, _1, wstring{ L"FJP" }, _2),
		bind(&decode_big, _1, wstring{ L"INC" }, _2),
		bind(&decode_big, _1, wstring{ L"IND" }, _2),
		bind(&decode_big, _1, wstring{ L"IXA" }, _2),
		bind(&decode_big, _1, wstring{ L"LAO" }, _2),
		bind(&decode_stringConstant, _1, wstring{ L"LSA" }, _2),
		bind(&decode_extended, _1, wstring{ L"LAE" }, _2),
		bind(&decode_big, _1, wstring{ L"MOV" }, _2),
		bind(&decode_big, _1, wstring{ L"LDO" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"SAS" }, _2),
		bind(&decode_big, _1, wstring{ L"SRO" }, _2),
		bind(&decode_case, _1, wstring{ L"XJP" }, _2),
		bind(&decode_return, _1, wstring{ L"RNP" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"CIP" }, _2),
		bind(&decode_compare, _1, wstring{ L"EQU" }, _2),
		bind(&decode_compare, _1, wstring{ L"GEQ" }, _2),
		bind(&decode_compare, _1, wstring{ L"GRT" }, _2),
		bind(&decode_intermediate, _1, wstring{ L"LDA" }, _2),
		bind(&decode_wordBlock, _1, wstring{ L"LDC" }, _2),
		bind(&decode_compare, _1, wstring{ L"LEQ" }, _2),
		bind(&decode_compare, _1, wstring{ L"LES" }, _2),
		bind(&decode_intermediate, _1, wstring{ L"LOD" }, _2),
		bind(&decode_compare, _1, wstring{ L"NEQ" }, _2),
		bind(&decode_intermediate, _1, wstring{ L"STR" }, _2),
		bind(&decode_jump, _1, wstring{ L"UJP" }, _2),
		bind(&decode_implied, _1, wstring{ L"LDP" }, _2),
		bind(&decode_implied, _1, wstring{ L"STP" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"LDM" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"STM" }, _2),
		bind(&decode_implied, _1, wstring{ L"LDB" }, _2),
		bind(&decode_implied, _1, wstring{ L"STB" }, _2),
		bind(&decode_doubleByte, _1, wstring{ L"IXP" }, _2),
		bind(&decode_return, _1, wstring{ L"RBP" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"CBP" }, _2),
		bind(&decode_implied, _1, wstring{ L"EQUI" }, _2),
		bind(&decode_implied, _1, wstring{ L"GEQI" }, _2),
		bind(&decode_implied, _1, wstring{ L"GRTI" }, _2),
		bind(&decode_big, _1, wstring{ L"LLA" }, _2),
		bind(&decode_word, _1, wstring{ L"LDCI" }, _2),
		bind(&decode_implied, _1, wstring{ L"LEQI" }, _2),
		bind(&decode_implied, _1, wstring{ L"LESI" }, _2),
		bind(&decode_big, _1, wstring{ L"LDL" }, _2),
		bind(&decode_implied, _1, wstring{ L"NEQI" }, _2),
		bind(&decode_big, _1, wstring{ L"STL" }, _2),
		bind(&decode_doubleByte, _1, wstring{ L"CXP" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"CLP" }, _2),
		bind(&decode_unsignedByte, _1, wstring{ L"CGP" }, _2),
		bind(&decode_packedConstant, _1, wstring{ L"LPA" }, _2),
		bind(&decode_extended, _1, wstring{ L"STE" }, _2),
		bind(&decode_implied, _1, wstring{ L"" }, _2),
		bind(&decode_jump, _1, wstring{ L"EFJ" }, _2),
		bind(&decode_jump, _1, wstring{ L"NFJ" }, _2),
		bind(&decode_big, _1, wstring{ L"BPT" }, _2),
		bind(&decode_implied, _1, wstring{ L"XIT" }, _2),
		bind(&decode_implied, _1, wstring{ L"NOP" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_1" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_2" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_3" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_4" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_5" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_6" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_7" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_8" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_9" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_10" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_11" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_12" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_13" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_14" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_15" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDL_16" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_1" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_2" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_3" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_4" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_5" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_6" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_7" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_8" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_9" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_10" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_11" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_12" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_13" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_14" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_15" }, _2),
		bind(&decode_implied, _1, wstring{ L"SLDO_16" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_0" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_1" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_2" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_3" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_4" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_5" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_6" }, _2),
		bind(&decode_implied, _1, wstring{ L"SIND_7" }, _2),
	};

	std::uint8_t const* PcodeProcedure::Disassembler::decode(std::uint8_t const* current) const {
		auto opcode = *current++;
		return dispatch[opcode](this, current);
	}

	PcodeProcedure::PcodeProcedure(CodePart& codePart, int procedureNumber, Range<std::uint8_t const> range) :
		base(codePart, procedureNumber, range),
		attributeTable{ reinterpret_cast<AttributeTable const*>(data.end() - sizeof(AttributeTable)) }
	{}

	std::uint8_t const* PcodeProcedure::getEnterIc() const {
		return derefSelfPtr(reinterpret_cast<std::uint8_t const*>(&attributeTable->enterIc));
	}

	std::uint8_t const* PcodeProcedure::getExitIc() const {
		return derefSelfPtr(reinterpret_cast<std::uint8_t const*>(&attributeTable->exitIc));
	}

	uint8_t const* PcodeProcedure::jtab(int index) const {
		return reinterpret_cast<uint8_t const*>(&attributeTable->procedureNumber) + index;
	}

	void PcodeProcedure::writeHeader(std::wostream& os) const {
		auto procBegin = data.begin();
		auto procLength = data.end() - data.begin();
		os << "Proc #" << dec << setfill(L' ') << left << setw(4) << procedureNumber << L" (";
		os << hex << setfill(L'0') << right << setw(4) << distance(codePart.begin(), procBegin) << ":" << setw(4) << distance(codePart.begin(), procBegin) + procLength - 1 << ")  P-Code (LSB)   ";
		os << setfill(L' ') << dec << left;
		os << L"Lex level = " << setw(4) << attributeTable->lexLevel;
		os << L"Parameters = " << setw(4) << attributeTable->paramaterSize;
		os << L"Variables = " << setw(4) << attributeTable->dataSize;
		os << endl;
	}

	void PcodeProcedure::disassemble(std::wostream& os, linkref_map_t& linkage) const {
		Disassembler disassember{ os, *this, linkage };
		uint8_t const* ic = data.begin();
		while (ic && ic < data.end()) {
			printIc(os, ic);
			ic = disassember.decode(ic);
		}
	}

	void PcodeProcedure::printIc(std::wostream& os, uint8_t const* current)  const {
		if (getEnterIc() == current) {
			os << L"ENTER  :" << endl;
		}
		if (getExitIc() == current) {
			os << L"EXIT   :" << endl;
		}
		os << L"   ";
		os << hex << setfill(L'0') << right << setw(4) << static_cast<int>(current - getProcBegin()) << L": ";
	}

}
