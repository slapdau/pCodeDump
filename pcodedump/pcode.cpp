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

#include "pcode.hpp"
#include "directory.hpp"
#include "types.hpp"
#include "textio.hpp"
#include "options.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <cstdint>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	PcodeProcedure::PcodeProcedure(CodeSegment & segment, int procedureNumber, std::uint8_t * procBegin, int procLength) :
		base(segment, procedureNumber, procBegin, procLength),
		rawAttributeTable{ reinterpret_cast<RawPcodeAttributeTable *>(procBegin + procLength - sizeof(RawPcodeAttributeTable)) }
	{}

	vector<PcodeProcedure::dispatch_t> PcodeProcedure::dispatch = {
		make_tuple(L"SDLC_0", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_1", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_2", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_3", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_4", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_5", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_6", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_7", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_8", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_9", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_10", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_11", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_12", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_13", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_14", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_15", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_16", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_17", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_18", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_19", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_20", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_21", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_22", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_23", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_24", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_25", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_26", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_27", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_28", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_29", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_30", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_31", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_32", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_33", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_34", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_35", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_36", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_37", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_38", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_39", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_40", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_41", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_42", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_43", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_44", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_45", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_46", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_47", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_48", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_49", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_50", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_51", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_52", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_53", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_54", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_55", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_56", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_57", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_58", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_59", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_60", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_61", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_62", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_63", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_64", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_65", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_66", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_67", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_68", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_69", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_70", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_71", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_72", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_73", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_74", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_75", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_76", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_77", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_78", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_79", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_80", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_81", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_82", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_83", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_84", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_85", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_86", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_87", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_88", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_89", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_90", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_91", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_92", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_93", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_94", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_95", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_96", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_97", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_98", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_99", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_100", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_101", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_102", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_103", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_104", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_105", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_106", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_107", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_108", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_109", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_110", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_111", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_112", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_113", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_114", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_115", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_116", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_117", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_118", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_119", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_120", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_121", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_122", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_123", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_124", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_125", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_126", &PcodeProcedure::decode_implied),
		make_tuple(L"SDLC_127", &PcodeProcedure::decode_implied),
		make_tuple(L"ABI", &PcodeProcedure::decode_implied),
		make_tuple(L"ABR", &PcodeProcedure::decode_implied),
		make_tuple(L"ADI", &PcodeProcedure::decode_implied),
		make_tuple(L"ADR", &PcodeProcedure::decode_implied),
		make_tuple(L"LAND", &PcodeProcedure::decode_implied),
		make_tuple(L"DIF", &PcodeProcedure::decode_implied),
		make_tuple(L"DVI", &PcodeProcedure::decode_implied),
		make_tuple(L"DVR", &PcodeProcedure::decode_implied),
		make_tuple(L"CHK", &PcodeProcedure::decode_implied),
		make_tuple(L"FLO", &PcodeProcedure::decode_implied),
		make_tuple(L"FLT", &PcodeProcedure::decode_implied),
		make_tuple(L"INN", &PcodeProcedure::decode_implied),
		make_tuple(L"INT", &PcodeProcedure::decode_implied),
		make_tuple(L"LOR", &PcodeProcedure::decode_implied),
		make_tuple(L"MODI", &PcodeProcedure::decode_implied),
		make_tuple(L"MPI", &PcodeProcedure::decode_implied),
		make_tuple(L"MPR", &PcodeProcedure::decode_implied),
		make_tuple(L"NGI", &PcodeProcedure::decode_implied),
		make_tuple(L"NGR", &PcodeProcedure::decode_implied),
		make_tuple(L"LNOT", &PcodeProcedure::decode_implied),
		make_tuple(L"SRS", &PcodeProcedure::decode_implied),
		make_tuple(L"SBI", &PcodeProcedure::decode_implied),
		make_tuple(L"SBR", &PcodeProcedure::decode_implied),
		make_tuple(L"SGS", &PcodeProcedure::decode_implied),
		make_tuple(L"SQI", &PcodeProcedure::decode_implied),
		make_tuple(L"SQR", &PcodeProcedure::decode_implied),
		make_tuple(L"STO", &PcodeProcedure::decode_implied),
		make_tuple(L"IXS", &PcodeProcedure::decode_implied),
		make_tuple(L"UNI", &PcodeProcedure::decode_implied),
		make_tuple(L"LDE", &PcodeProcedure::decode_extended),
		make_tuple(L"CSP", &PcodeProcedure::decode_callStandardProc),
		make_tuple(L"LDCN", &PcodeProcedure::decode_implied),
		make_tuple(L"ADJ", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"FJP", &PcodeProcedure::decode_jump),
		make_tuple(L"INC", &PcodeProcedure::decode_big),
		make_tuple(L"IND", &PcodeProcedure::decode_big),
		make_tuple(L"IXA", &PcodeProcedure::decode_big),
		make_tuple(L"LAO", &PcodeProcedure::decode_big),
		make_tuple(L"LSA", &PcodeProcedure::decode_stringConstant),
		make_tuple(L"LAE", &PcodeProcedure::decode_extended),
		make_tuple(L"MOV", &PcodeProcedure::decode_big),
		make_tuple(L"LDO", &PcodeProcedure::decode_big),
		make_tuple(L"SAS", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"SRO", &PcodeProcedure::decode_big),
		make_tuple(L"XJP", &PcodeProcedure::decode_case),
		make_tuple(L"RNP", &PcodeProcedure::decode_return),
		make_tuple(L"CIP", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"EQU", &PcodeProcedure::decode_compare),
		make_tuple(L"GEQ", &PcodeProcedure::decode_compare),
		make_tuple(L"GRT", &PcodeProcedure::decode_compare),
		make_tuple(L"LDA", &PcodeProcedure::decode_intermediate),
		make_tuple(L"LDC", &PcodeProcedure::decode_wordBlock),
		make_tuple(L"LEQ", &PcodeProcedure::decode_compare),
		make_tuple(L"LES", &PcodeProcedure::decode_compare),
		make_tuple(L"LOD", &PcodeProcedure::decode_intermediate),
		make_tuple(L"NEQ", &PcodeProcedure::decode_compare),
		make_tuple(L"STR", &PcodeProcedure::decode_intermediate),
		make_tuple(L"UJP", &PcodeProcedure::decode_jump),
		make_tuple(L"LDP", &PcodeProcedure::decode_implied),
		make_tuple(L"STP", &PcodeProcedure::decode_implied),
		make_tuple(L"LDM", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"STM", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"LDB", &PcodeProcedure::decode_implied),
		make_tuple(L"STB", &PcodeProcedure::decode_implied),
		make_tuple(L"IXP", &PcodeProcedure::decode_doubleByte),
		make_tuple(L"RBP", &PcodeProcedure::decode_return),
		make_tuple(L"CBP", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"EQUI", &PcodeProcedure::decode_implied),
		make_tuple(L"GEQI", &PcodeProcedure::decode_implied),
		make_tuple(L"GRTI", &PcodeProcedure::decode_implied),
		make_tuple(L"LLA", &PcodeProcedure::decode_big),
		make_tuple(L"LDCI", &PcodeProcedure::decode_word),
		make_tuple(L"LEQI", &PcodeProcedure::decode_implied),
		make_tuple(L"LESI", &PcodeProcedure::decode_implied),
		make_tuple(L"LDL", &PcodeProcedure::decode_big),
		make_tuple(L"NEQI", &PcodeProcedure::decode_implied),
		make_tuple(L"STL", &PcodeProcedure::decode_big),
		make_tuple(L"CXP", &PcodeProcedure::decode_doubleByte),
		make_tuple(L"CLP", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"CGP", &PcodeProcedure::decode_unsignedByte),
		make_tuple(L"LPA", &PcodeProcedure::decode_packedConstant),
		make_tuple(L"STE", &PcodeProcedure::decode_extended),
		make_tuple(L"", &PcodeProcedure::decode_implied),
		make_tuple(L"EFJ", &PcodeProcedure::decode_jump),
		make_tuple(L"NFJ", &PcodeProcedure::decode_jump),
		make_tuple(L"BPT", &PcodeProcedure::decode_big),
		make_tuple(L"XIT", &PcodeProcedure::decode_implied),
		make_tuple(L"NOP", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_1", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_2", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_3", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_4", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_5", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_6", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_7", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_8", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_9", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_10", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_11", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_12", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_13", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_14", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_15", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDL_16", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_1", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_2", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_3", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_4", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_5", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_6", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_7", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_8", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_9", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_10", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_11", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_12", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_13", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_14", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_15", &PcodeProcedure::decode_implied),
		make_tuple(L"SLDO_16", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_0", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_1", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_2", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_3", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_4", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_5", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_6", &PcodeProcedure::decode_implied),
		make_tuple(L"SIND_7", &PcodeProcedure::decode_implied),
	};


	std::uint8_t * PcodeProcedure::getEnterIc() const {
		return derefSelfPtr(reinterpret_cast<std::uint8_t *>(&rawAttributeTable->enterIc));
	}

	std::uint8_t * PcodeProcedure::getExitIc() const {
		return derefSelfPtr(reinterpret_cast<std::uint8_t *>(&rawAttributeTable->exitIc));
	}

	uint8_t * PcodeProcedure::jtab(int index) const {
		return reinterpret_cast<uint8_t *>(&rawAttributeTable->procedureNumber) + index;
	}

	void PcodeProcedure::writeHeader(uint8_t* segBegin, std::wostream& os) const {
		os << "Proc #" << dec << setfill(L' ') << left << setw(4) << procedureNumber << L" (";
		os << hex << setfill(L'0') << right << setw(4) << distance(segBegin, procBegin) << ":" << setw(4) << distance(segBegin, procBegin) + procLength << ")  P-Code (LSB)   ";
		os << setfill(L' ') << dec << left;
		os << L"Lex level = " << setw(4) << rawAttributeTable->lexLevel;
		os << L"Parameters = " << setw(4) << rawAttributeTable->paramaterSize;
		os << L"Variables = " << setw(4) << rawAttributeTable->dataSize;
		os << endl;
	}

	void PcodeProcedure::disassemble(uint8_t* segBegin, std::wostream& os) const {
		if (!rawAttributeTable->procedureNumber) {
			return;
		}
		uint8_t * ic = procBegin;
		while (ic && ic < (procBegin + procLength)) {
			wstring opcode;
			decode_function_t decode_function;
			tie(opcode, decode_function) = dispatch[*ic];
			ic = (this->*decode_function)(os, opcode, ic);
		}
	}

	void PcodeProcedure::printIc(std::wostream& os, uint8_t * current)  const {
		if (getEnterIc() == current) {
			os << L"ENTER  :" << endl;
		}
		if (getExitIc() == current) {
			os << L"EXIT   :" << endl;
		}
		os << L"   ";
		os << hex << setfill(L'0') << right << setw(4) << static_cast<int>(current - getProcBegin()) << L": ";
	}

	uint8_t * PcodeProcedure::decode_implied(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current);
		os << opCode << endl;
		return current + 1;
	}

	/* ub */
	uint8_t * PcodeProcedure::decode_unsignedByte(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(9) << opCode << dec << *(current + 1) << endl;
		return current + 2;
	}

	/* b */
	uint8_t * PcodeProcedure::decode_big(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
		int value = *current++;
		if (value & 0x80) {
			value = ((value & 0x7f) << 8) + *current++;
		}
		os << setfill(L' ') << left << setw(9) << opCode << dec << value << endl;
		return current;
	}

	/* db, b */
	uint8_t * PcodeProcedure::decode_intermediate(std::wostream& os, wstring &opCode, uint8_t * current) const {
		printIc(os, current++);
		int linkCount = *current++;
		int offset = *current++;
		if (offset & 0x80) {
			offset = ((offset & 0x7f) << 8) + *current++;
		}
		os << setfill(L' ') << left << setw(9) << opCode << dec << linkCount << L", " << offset << endl;
		return current;
	}

	/* ub, b */
	uint8_t * PcodeProcedure::decode_extended(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
		int dataSegment = *current++;
		int offset = *current++;
		if (offset & 0x80) {
			offset = ((offset & 0x7f) << 8) + *current++;
		}
		os << setfill(L' ') << left << setw(9) << opCode << dec << dataSegment << L", " << offset << endl;
		return current;
	}

	/* w */
	uint8_t * PcodeProcedure::decode_word(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
		little_int16_t * value = reinterpret_cast<little_int16_t *>(current);
		current += sizeof(little_int16_t);
		os << setfill(L' ') << left << setw(9) << opCode << dec << *value << endl;
		return current;
	}

	void convertToReal(std::wostream& os, little_int16_t const * words) {
		uint16_t buf[2];
		buf[1] = words[0];
		buf[0] = words[1];
		os << *reinterpret_cast<float *>(buf);
	}

	/* ub, word aligned block of words */
	uint8_t * PcodeProcedure::decode_wordBlock(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
		int total = *current;
		if (reinterpret_cast<long long>(current) & 0x1) {
			current++;
		}
		os << setfill(L' ') << left << setw(9) << opCode << dec << total;
		if (total == 2) {
			os << L"                    ; ";
			convertToReal(os, reinterpret_cast<little_int16_t *>(current));
		}
		os << endl;
		for (int count = 0; count != total; ++count) {
			little_int16_t * value = reinterpret_cast<little_int16_t *>(current);
			current += sizeof(little_int16_t);
			os << setfill(L' ') << setw(18) << L"" << *value << endl;
		}
		return current;
	}

	/* ub, <chars> */
	uint8_t * PcodeProcedure::decode_stringConstant(std::wostream& os, wstring &opCode, uint8_t * current) const {
		printIc(os, current++);
		uint8_t count = *current++;
		os << setfill(L' ') << left << setw(9) << opCode << dec << count << endl;
		uint8_t * finish = current + count;
		FmtSentry<wostream::char_type> sentry{ wcout };
		while (current != finish) {
			uint8_t * next = distance(current, finish) >= 80 ? current + 80 : finish;
			wcout << L"                  ";
			line_chardump(current, next);
			current = next;
			wcout << endl;
		}
		return finish;
	}

	/* ub, <bytes> */
	uint8_t * PcodeProcedure::decode_packedConstant(std::wostream& os, wstring &opCode, uint8_t * current) const {
		printIc(os, current++);
		uint8_t count = *current++;
		os << setfill(L' ') << left << setw(9) << opCode << dec << count << endl;
		hexdump(wstring{ L"                  " }, current, current + count);
		current += count;
		return current;
	}

	/* sb */
	uint8_t * PcodeProcedure::decode_jump(std::wostream& os, wstring &opCode, uint8_t * current) const {
		printIc(os, current++);
		auto offset = getNext<int8_t>(current);
		intptr_t address;
		if (offset >= 0) {
			address = (current + offset) - getProcBegin();
		} else {
			address = derefSelfPtr(jtab(offset)) - getProcBegin();
		}
		os << setfill(L' ') << left << setw(9) << opCode << L"(" << hex << setfill(L'0') << right << setw(4) << address << L")" << endl;
		return current;
	}

	/* db */
	uint8_t * PcodeProcedure::decode_return(std::wostream& os, wstring &opCode, uint8_t * current) const {
		printIc(os, current++);
		os << opCode << endl;
		return nullptr;
	}

	/* ub, ub */
	uint8_t * PcodeProcedure::decode_doubleByte(std::wostream& os, wstring &opCode, uint8_t * current) const {
		printIc(os, current++);
		int value_1 = *current++;
		int value_2 = *current++;
		os << setfill(L' ') << left << setw(9) << opCode << dec << value_1 << L", " << value_2 << endl;
		return current;
	}

	/* word aligned -> idx_min, idx_max, (uj sb), table */
	uint8_t * PcodeProcedure::decode_case(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
		if (reinterpret_cast<uintptr_t>(current) & 0x1) {
			current++;
		}
		auto min = getNext<little_int16_t>(current);
		auto max = getNext<little_int16_t>(current);
		current++; // Skip the UJP opcode
		auto offset = getNext<int8_t>(current);
		intptr_t address;
		if (offset >= 0) {
			address = (current + offset) - getProcBegin();
		} else {
			address = derefSelfPtr(jtab(offset)) - getProcBegin();
		}
		os << setfill(L' ') << left << setw(9) << opCode << dec << min << ", " << max << " (" << hex << setfill(L'0') << right << setw(4) << address << ")" << endl;
		for (int count = min; count != max + 1; ++count) {
			address = derefSelfPtr(current) - getProcBegin();
			getNext<little_int16_t>(current);
			os << setfill(L' ') << setw(18) << L"" << L"(" << hex << setfill(L'0') << right << setw(4) << address << L")" << endl;
		}
		return current;
	}

	namespace {

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
	#if 0
			// These are actually implemented in the transcendental intrinsic unit.
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

	}

	/* CSP ub */
	uint8_t * PcodeProcedure::decode_callStandardProc(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
		int standardProcNumber = *current++;
		os << setfill(L' ') << left << setw(9) << opCode << dec << setw(6) << standardProcNumber;
		if (standardProcs.count(standardProcNumber)) {
			os << L"; " << standardProcs[standardProcNumber];
		}
		os << endl;
		return current;
	}

	/* 2-reals, 4-strings, 6-booleans, 8-sets, 10-byte arrays, 12-words. 10 and 12 have b as well */
	uint8_t * PcodeProcedure::decode_compare(std::wostream& os, wstring &opCode, uint8_t * current)  const {
		printIc(os, current++);
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

	PcodeSegment::PcodeSegment(SegmentDirectoryEntry & directoryEntry, std::uint8_t * segBegin, int segLength) :
		base(directoryEntry, segBegin, segLength)
	{
		entries = this->initProcedures();
	}

	/* Get the procedure code memory ranges and construct a vector of procedure objedts. P-code segments only
	   have p-code procedures. */
	unique_ptr<Procedures> PcodeSegment::initProcedures() {
		auto procRange = getProcRanges();
		auto result = make_unique<Procedures>();
		transform(std::begin(procRange), std::end(procRange), back_inserter(*result), [this](const auto & value) {
			uint8_t * start;
			int procNumber, length;
			tie(procNumber, start, length) = value;
			return make_shared<PcodeProcedure>(*this, procNumber + 1, start, length);
		});
		return result;
	}

	void PcodeSegment::disassemble(std::wostream& os) const {
		Procedures procs{ *entries };
		if (addressOrder) {
			sort(std::begin(procs), std::end(procs), [](const auto & left, const auto & right) { return left->getProcBegin() < right->getProcBegin(); });
		}
		for (auto & entry : procs) {
			entry->writeHeader(segBegin, os);
			if (disasmProcs) {
				entry->disassemble(segBegin, os);
				os << endl;
			}
		}
	}

}
