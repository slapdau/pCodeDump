#include "directory.hpp"
#include "native6502.hpp"
#include "pcode.hpp"
#include "types.hpp"
#include "options.hpp"

#include <iterator>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	/* Format a sequence of bytes as a string of space separated 2-digit hex values. */
	namespace {
		wstring toHexString(uint8_t *begin, uint8_t *end) {
			wostringstream buff;
			buff << hex << uppercase << setfill(L'0') << right;
			if (begin != end) {
				buff << setw(2) << *begin;
			}
			for (uint8_t *current = begin + 1; current != end; ++current) {
				buff << L" " << setw(2) << *current;
			}
			return buff.str();
		}

	}

	/* Read one of the 4 6502 procedure relocation tables. Return a pointer to the start of the table. */
	uint8_t * Native6502Procedure::readRelocations(vector<int> &table, uint8_t * rawTable) {
		uint8_t * current = rawTable;
		current -= sizeof(little_uint16_t);
		int total = *reinterpret_cast<little_uint16_t *>(current);
		for (int count = 0; count != total; ++count) {
			current -= sizeof(little_uint16_t);
			int address = defererenceSelfPtr(procBegin, current);
			table.push_back(address);
		}
		return current;
	}

	/* Format a 16-bit absolute address for display.  The address is embedded in a 6502 instruction.
	   If the address is referred to by either the segment or interpreter relocation tables, indicate
	   this in the formatting. */
	wstring Native6502Procedure::formatAbsoluteAddress(uint8_t * address) const {
		auto value = *reinterpret_cast<little_uint16_t *>(address);
		auto offset = distance(procBegin, address);
		wostringstream result;
		if (find(begin(segRelocations), end(segRelocations), offset) != segRelocations.end()) {
			result << L"Seg#" << dec << rawAttributeTable->relocationSeg << "+";
		} else if (find(begin(interpRelocations), end(interpRelocations), offset) != interpRelocations.end()) {
			result << L".interp+";
		}
		result << L"$" << uppercase << hex << setfill(L'0') << right << setw(4) << value;
		return result.str();
	}

	/* Adjust an absolute address pointed to by the procedure relocation table so that the stored value is
	   relative to segment base.  When 6502 is disassembled, the instruction addresses are displayed
	   related to the segment base, so this enable inspection of the control flow. */
	void Native6502Procedure::relocateProcAddress(int address) {
		uint8_t * addressPtr = procBegin + address;
		auto value = reinterpret_cast<little_uint16_t *>(addressPtr);
		*value = *value + static_cast<uint16_t>(distance(segment.begin(), procBegin));
	}

	Native6502Procedure::Native6502Procedure(CodeSegment  & segment, int procedureNumber, std::uint8_t * procBegin, int procLength) :
		base(segment, procedureNumber, procBegin, procLength),
		rawAttributeTable{ reinterpret_cast<RawNative6502AttributeTable *>(procBegin + procLength - sizeof(RawNative6502AttributeTable)) }
	{
		this->procEnd = procBegin + procLength - sizeof(RawNative6502AttributeTable);
		if (this->rawAttributeTable->procedureNumber == 0) {
			for (auto table : { &baseRelocations, &segRelocations, &procRelocations, &interpRelocations }) {
				procEnd = readRelocations(*table, procEnd);
			}
		}
		for (auto address : procRelocations) {
			relocateProcAddress(address);
		}
	}

	/* Dispatch for opcodes.  For each opcode, the table contains the mneumonic and a pointer
	   the the correct method to decode the address mode.
	   
	   This vector is initialised with 6502 instructions only. Later, based on program options,
	   it might be patched with opcodes for other processors. */
	std::vector<Native6502Procedure::dispatch_t> Native6502Procedure::dispatch = {
		// 0x00
		make_tuple(L"BRK", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_zeropage),
		make_tuple(L"ASL", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"PHP", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_immedidate),
		make_tuple(L"ASL", &Native6502Procedure::decode_accumulator),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_absolute),
		make_tuple(L"ASL", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x10
		make_tuple(L"BPL", &Native6502Procedure::decode_relative),
		make_tuple(L"ORA", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"ASL", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CLC", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ORA", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"ASL", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x20
		make_tuple(L"JSR", &Native6502Procedure::decode_absolute),
		make_tuple(L"AND", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"BIT", &Native6502Procedure::decode_zeropage),
		make_tuple(L"AND", &Native6502Procedure::decode_zeropage),
		make_tuple(L"ROL", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"PLP", &Native6502Procedure::decode_implied),
		make_tuple(L"AND", &Native6502Procedure::decode_immedidate),
		make_tuple(L"ROL", &Native6502Procedure::decode_accumulator),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"BIT", &Native6502Procedure::decode_absolute),
		make_tuple(L"AND", &Native6502Procedure::decode_absolute),
		make_tuple(L"ROL", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x30
		make_tuple(L"BMI", &Native6502Procedure::decode_relative),
		make_tuple(L"AND", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"AND", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"ROL", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"SEC", &Native6502Procedure::decode_implied),
		make_tuple(L"AND", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"AND", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"ROL", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x40
		make_tuple(L"RTI", &Native6502Procedure::decode_implied),
		make_tuple(L"EOR", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"EOR", &Native6502Procedure::decode_zeropage),
		make_tuple(L"LSR", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"PHA", &Native6502Procedure::decode_implied),
		make_tuple(L"EOR", &Native6502Procedure::decode_immedidate),
		make_tuple(L"LSR", &Native6502Procedure::decode_accumulator),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"JMP", &Native6502Procedure::decode_absolute),
		make_tuple(L"EOR", &Native6502Procedure::decode_absolute),
		make_tuple(L"LSR", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x50
		make_tuple(L"BVC", &Native6502Procedure::decode_relative),
		make_tuple(L"EOR", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"EOR", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"LSR", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CLI", &Native6502Procedure::decode_implied),
		make_tuple(L"EOR", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"EOR", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"LSR", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x60
		make_tuple(L"RTS", &Native6502Procedure::decode_implied),
		make_tuple(L"ADC", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ADC", &Native6502Procedure::decode_zeropage),
		make_tuple(L"ROR", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"PLA", &Native6502Procedure::decode_implied),
		make_tuple(L"ADC", &Native6502Procedure::decode_immedidate),
		make_tuple(L"ROR", &Native6502Procedure::decode_accumulator),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"JMP", &Native6502Procedure::decode_absoluteindirect),
		make_tuple(L"ADC", &Native6502Procedure::decode_absolute),
		make_tuple(L"ROR", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x70
		make_tuple(L"BVS", &Native6502Procedure::decode_relative),
		make_tuple(L"ADC", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ADC", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"ROR", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"SEI", &Native6502Procedure::decode_implied),
		make_tuple(L"ADC", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"ADC", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"ROR", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x80
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"STA", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"STY", &Native6502Procedure::decode_zeropage),
		make_tuple(L"STA", &Native6502Procedure::decode_zeropage),
		make_tuple(L"STX", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"DEY", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"TXA", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"STY", &Native6502Procedure::decode_absolute),
		make_tuple(L"STA", &Native6502Procedure::decode_absolute),
		make_tuple(L"STX", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0x90
		make_tuple(L"BCC", &Native6502Procedure::decode_relative),
		make_tuple(L"STA", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"STY", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"STA", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"STX", &Native6502Procedure::decode_zeropageindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"TYA", &Native6502Procedure::decode_implied),
		make_tuple(L"STA", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"TXS", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"STA", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0xA0
		make_tuple(L"LDY", &Native6502Procedure::decode_immedidate),
		make_tuple(L"LDA", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"LDX", &Native6502Procedure::decode_immedidate),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"LDY", &Native6502Procedure::decode_zeropage),
		make_tuple(L"LDA", &Native6502Procedure::decode_zeropage),
		make_tuple(L"LDX", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"TAY", &Native6502Procedure::decode_implied),
		make_tuple(L"LDA", &Native6502Procedure::decode_immedidate),
		make_tuple(L"TAX", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"LDY", &Native6502Procedure::decode_absolute),
		make_tuple(L"LDA", &Native6502Procedure::decode_absolute),
		make_tuple(L"LDX", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0xB0
		make_tuple(L"BCS", &Native6502Procedure::decode_relative),
		make_tuple(L"LDA", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"LDY", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"LDA", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"LDX", &Native6502Procedure::decode_zeropageindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CLV", &Native6502Procedure::decode_implied),
		make_tuple(L"LDA", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"TSX", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"LDY", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"LDA", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"LDX", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0xC0
		make_tuple(L"CPY", &Native6502Procedure::decode_immedidate),
		make_tuple(L"CMP", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CPY", &Native6502Procedure::decode_zeropage),
		make_tuple(L"CMP", &Native6502Procedure::decode_zeropage),
		make_tuple(L"DEC", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"INY", &Native6502Procedure::decode_implied),
		make_tuple(L"CMP", &Native6502Procedure::decode_immedidate),
		make_tuple(L"DEX", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CPY", &Native6502Procedure::decode_absolute),
		make_tuple(L"CMP", &Native6502Procedure::decode_absolute),
		make_tuple(L"DEC", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0xD0
		make_tuple(L"BNE", &Native6502Procedure::decode_relative),
		make_tuple(L"CMP", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CMP", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"DEC", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CLD", &Native6502Procedure::decode_implied),
		make_tuple(L"CMP", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CMP", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"DEC", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0xE0
		make_tuple(L"CPX", &Native6502Procedure::decode_immedidate),
		make_tuple(L"SBC", &Native6502Procedure::decode_indexedindirect),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CPX", &Native6502Procedure::decode_zeropage),
		make_tuple(L"SBC", &Native6502Procedure::decode_zeropage),
		make_tuple(L"INC", &Native6502Procedure::decode_zeropage),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"INX", &Native6502Procedure::decode_implied),
		make_tuple(L"SBC", &Native6502Procedure::decode_immedidate),
		make_tuple(L"NOP", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"CPX", &Native6502Procedure::decode_absolute),
		make_tuple(L"SBC", &Native6502Procedure::decode_absolute),
		make_tuple(L"INC", &Native6502Procedure::decode_absolute),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		// 0xF0
		make_tuple(L"BEQ", &Native6502Procedure::decode_relative),
		make_tuple(L"SBC", &Native6502Procedure::decode_indirectindexed),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"SBC", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"INC", &Native6502Procedure::decode_zeropageindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"SED", &Native6502Procedure::decode_implied),
		make_tuple(L"SBC", &Native6502Procedure::decode_absoluteindexedy),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
		make_tuple(L"SBC", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"INC", &Native6502Procedure::decode_absoluteindexedx),
		make_tuple(L"???", &Native6502Procedure::decode_implied),
	};

	/* Opcode patches for the dispatch table if the 65c02 is chosen. */
	std::map<int, Native6502Procedure::dispatch_t> Native6502Procedure::dispatch_65c02 = {
		{0x04,make_tuple(L"TSB", &Native6502Procedure::decode_zeropage)},
		{0x0C,make_tuple(L"TSB", &Native6502Procedure::decode_absolute)},
		{0x12,make_tuple(L"ORA", &Native6502Procedure::decode_zeropageindirect)},
		{0x14,make_tuple(L"TRB", &Native6502Procedure::decode_zeropage)},
		{0x1A,make_tuple(L"INC", &Native6502Procedure::decode_accumulator)},
		{0x1C,make_tuple(L"TRB", &Native6502Procedure::decode_absolute)},
		{0x32,make_tuple(L"AND", &Native6502Procedure::decode_zeropage)},
		{0x34,make_tuple(L"BIT", &Native6502Procedure::decode_zeropageindexedx)},
		{0x3A,make_tuple(L"DEC", &Native6502Procedure::decode_accumulator)},
		{0x3C,make_tuple(L"BIT", &Native6502Procedure::decode_absoluteindexedx)},
		{0x52,make_tuple(L"EOR", &Native6502Procedure::decode_zeropage)},
		{0x5A,make_tuple(L"PHY", &Native6502Procedure::decode_implied)},
		{0x64,make_tuple(L"STZ", &Native6502Procedure::decode_zeropage)},
		{0x72,make_tuple(L"ADC", &Native6502Procedure::decode_zeropage)},
		{0x74,make_tuple(L"STZ", &Native6502Procedure::decode_zeropageindexedx)},
		{0x7A,make_tuple(L"PLY", &Native6502Procedure::decode_implied)},
		{0x7C,make_tuple(L"JMP", &Native6502Procedure::decode_absoluteindirectindexed)},
		{0x80,make_tuple(L"BRA", &Native6502Procedure::decode_relative)},
		{0x89,make_tuple(L"BIT", &Native6502Procedure::decode_immedidate)},
		{0x92,make_tuple(L"STA", &Native6502Procedure::decode_zeropage)},
		{0x9C,make_tuple(L"STZ", &Native6502Procedure::decode_absolute)},
		{0x9E,make_tuple(L"STZ", &Native6502Procedure::decode_absoluteindexedx)},
		{0xB2,make_tuple(L"LDA", &Native6502Procedure::decode_zeropage)},
		{0xD2,make_tuple(L"CMP", &Native6502Procedure::decode_zeropage)},
		{0xDA,make_tuple(L"PHX", &Native6502Procedure::decode_implied)},
		{0xF2,make_tuple(L"SBC", &Native6502Procedure::decode_zeropage)},
		{0xFA,make_tuple(L"PLX", &Native6502Procedure::decode_implied)},
	};

	/* Check the nominated CPU type, and patch the opcode decoding dispatch table if necessary. */
	void Native6502Procedure::initialiseCpu() {
		if (cpu == cpu_t::_65c02) {
			for (auto entry : dispatch_65c02) {
				int instruction;
				dispatch_t dispatchUpdate;
				tie(instruction, dispatchUpdate) = entry;
				dispatch[instruction] = dispatchUpdate;
			}
		}
	}

	void Native6502Procedure::writeHeader(std::uint8_t * segBegin, std::wostream & os) const {
		os << "Proc #" << dec << setfill(L' ') << left << setw(4) << procedureNumber << L" (";
		os << hex << setfill(L'0') << right << setw(4) << distance(segBegin, procBegin) << ":" << setw(4) << distance(segBegin, procBegin) + procLength << ") Native (6502)  ";
		os << endl;
	}

	/* Write a disassembly of the procedure to an output stream. */
	void Native6502Procedure::disassemble(std::uint8_t * segBegin, std::wostream & os) const {
		uint8_t * ic = procBegin;
		os << uppercase;
		while (ic && ic < procEnd) {
			wstring opcode;
			decode_function_t decode_function;
			tie(opcode, decode_function) = dispatch[*ic];
			ic = (this->*decode_function)(os, opcode, ic);
		}
	}

	int Native6502Procedure::getEnterIc() const {
		return defererenceSelfPtr(procBegin, &rawAttributeTable->enterIc);
	}

	/* Write the instruction address, relative to the segment start.  Indicate the procedure entry point. */
	void Native6502Procedure::printIc(std::wostream & os, std::uint8_t * current) const {
		int ic = static_cast<int>(distance(procBegin, current));
		if (getEnterIc() == distance(procBegin, current)) {
			os << L"  ENTER:" << endl;
		}
		os << L"   ";
		os << hex << setfill(L'0') << right << setw(4) << distance(segment.begin(), current) << L": ";
	}

	std::uint8_t * Native6502Procedure::decode_implied(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 1);
		os << opCode << endl;
		return current + 1;
	}

	std::uint8_t * Native6502Procedure::decode_immedidate(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" #$" << hex << setfill(L'0') << right << setw(2) << *value << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_accumulator(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 1);
		os << opCode << L" A" << endl;
		return current + 1;
	}

	std::uint8_t * Native6502Procedure::decode_absolute(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
		os << opCode << L" " << formatAbsoluteAddress(current + 1) << endl;
		return current + 3;
	}

	std::uint8_t * Native6502Procedure::decode_absoluteindirect(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
		auto value = reinterpret_cast<little_uint16_t *>(current + 1);
		os << opCode << L" (" << formatAbsoluteAddress(current + 1) << L")" << endl;
		return current + 3;
	}

	std::uint8_t * Native6502Procedure::decode_absoluteindirectindexed(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
		auto value = reinterpret_cast<little_uint16_t *>(current + 1);
		os << opCode << L" (" << formatAbsoluteAddress(current + 1) << L",X)" << endl;
		return current + 3;
	}

	std::uint8_t * Native6502Procedure::decode_zeropage(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" $" << hex << setfill(L'0') << right << setw(2) << *value << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_zeropageindirect(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" ($" << hex << setfill(L'0') << right << setw(2) << *value << L")" << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_absoluteindexedx(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
		os << opCode << L" " << formatAbsoluteAddress(current + 1) << L",X" << endl;
		return current + 3;
	}

	std::uint8_t * Native6502Procedure::decode_absoluteindexedy(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 3);
		os << opCode << L" " << formatAbsoluteAddress(current + 1) << L",Y" << endl;
		return current + 3;
	}

	std::uint8_t * Native6502Procedure::decode_zeropageindexedx(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" $" << hex << setfill(L'0') << right << setw(2) << *value << L",X" << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_zeropageindexedy(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" $" << hex << setfill(L'0') << right << setw(2) << *value << L",Y" << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_relative(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_int8_t *>(current + 1);
		os << opCode << L" $" << hex << setfill(L'0') << right << setw(4) << distance(segment.begin(), current + 2 + *value) << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_indexedindirect(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" ($" << hex << setfill(L'0') << right << setw(2) << *value << L",X)" << endl;
		return current + 2;
	}

	std::uint8_t * Native6502Procedure::decode_indirectindexed(std::wostream & os, std::wstring & opCode, std::uint8_t * current) const {
		printIc(os, current);
		os << setfill(L' ') << left << setw(10) << toHexString(current, current + 2);
		auto value = reinterpret_cast<little_uint8_t *>(current + 1);
		os << opCode << L" ($" << hex << setfill(L'0') << right << setw(2) << *value << L"),Y" << endl;
		return current + 2;
	}

	Native6502Segment::Native6502Segment(SegmentDirectoryEntry & directoryEntry, std::uint8_t * segBegin, int segLength) :
		base(directoryEntry, segBegin, segLength), entries{ initProcedures() }
	{}

	void Native6502Segment::disassemble(std::wostream & os) const {
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

	/* Get the memory ranges for procedures in this segment and construct a suitable Procedure object
	   for each.  Native code segments can contain both p-code and native code. */
	std::unique_ptr<Procedures> Native6502Segment::initProcedures() {
		auto procRange = getProcRanges();
		auto result = make_unique<Procedures>();
		transform(std::begin(procRange), std::end(procRange), back_inserter(*result), [this](const auto & value) ->shared_ptr<Procedure> {
			uint8_t * start;
			int procNumber, length;
			tie(procNumber, start, length) = value;
			// If the procedure number is recorded as 0, then it's a native procedure.
			if (*(start + length - 2)) {
				return make_shared<PcodeProcedure>(*this, procNumber + 1, start, length);
			} else {
				return make_shared<Native6502Procedure>(*this, procNumber + 1, start, length);
			}
		});
		return result;
	}

}
