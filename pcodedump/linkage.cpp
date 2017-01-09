#include "linkage.hpp"
#include "directory.hpp"
#include <iostream>
#include <string>
#include <map>
#include <iomanip>
#include <boost/endian/arithmetic.hpp>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	LinkageSegment::LinkageSegment(SegmentDirectoryEntry & directoryEntry, const std::uint8_t * linkage) :
		directoryEntry{ directoryEntry }, linkage{ linkage }
	{
	}

	/* Write out linkage records. */
	void LinkageSegment::write(std::wostream & os) const
	{
		uint8_t const * current = linkage;
		os << L"Linkage records:" << endl;
		while (current) {
			wstring name{ current, current + 8 };
			current += 8;
			int linkageTypeValue = *reinterpret_cast<little_int16_t const *>(current);
			LinkageType linkageType = static_cast<LinkageType>(linkageTypeValue);
			current += sizeof(little_int16_t);
			wstring linkageTypeName;
			decode_function_t decode_function;
			tie(linkageTypeName, decode_function) = linkageNames[linkageType];
			os << L"  " << name << L" " << setfill(L' ') << left << setw(20) << linkageTypeName << L" ";
			current = (this->*decode_function)(os, current);
		}
	}

	namespace {

		enum class OperandFormat { word, byte, big };

		map<OperandFormat, wstring> operandFormatNames = {
			{ OperandFormat::big,  L"big" },
			{ OperandFormat::byte, L"byte" },
			{ OperandFormat::word, L"word" },
		};

		std::wostream& operator<<(std::wostream& os, const OperandFormat& value) {
			os << operandFormatNames[value];
			return os;
		}

	}

	/* Write out a list of address references back into the procedure code and return the next
	   address beyond the array storage. */
	uint8_t const * LinkageSegment::decode_referenceArray(wostream & os, uint8_t const *current, int numberOfRefs) const {
		auto pointers = reinterpret_cast<little_int16_t const *>(current);
		os << hex << setfill(L'0') << right;
		for (int index = 0; index != numberOfRefs; ++index) {
			if (index % 8 == 0) {
				if (index != 0) {
					os << endl;
				}
				os << "    ";
			}
			os << setw(4) << pointers[index] << L" ";
		}
		os << endl;
		return current + (numberOfRefs / 8 + 1) * 8 * sizeof(little_int16_t);
	}

	uint8_t const * LinkageSegment::decode_reference(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << static_cast<OperandFormat>(static_cast<int>(parameters[0])) << endl;
		return decode_referenceArray(os, current + sizeof(little_int16_t) * 3, parameters[1]);
	}

	uint8_t const * LinkageSegment::decode_privateReference(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << static_cast<OperandFormat>(static_cast<int>(parameters[0])) << L" (" << parameters[2] << L" words)" << endl;
		return decode_referenceArray(os, current + sizeof(little_int16_t) * 3, parameters[1]);
	}

	uint8_t const * LinkageSegment::decode_globalDefinition(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"#" << parameters[0] << L", IC=" << parameters[1] << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageSegment::decode_publicDefinition(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"base = " << parameters[0] << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageSegment::decode_constantDefinition(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"= " << parameters[0] << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageSegment::decode_externalRoutine(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"#" << parameters[0] << " (" << parameters[1] << L" words)" << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageSegment::decode_endOfFile(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		if (directoryEntry.getSegmentKind() != SegmentKind::seprtseg) {
			os << dec << parameters[0] << " global words";
			if (directoryEntry.getSegmentKind() == SegmentKind::unlinkedIntrins) {
				os << ", private data seg #" << parameters[1];
			}
			os << endl;
		}
		return nullptr;
	}

	map<LinkageType, tuple<wstring, LinkageSegment::decode_function_t>> LinkageSegment::linkageNames = {
		{ LinkageType::eofMark,  make_tuple(L"end of linkage", &LinkageSegment::decode_endOfFile) },
		{ LinkageType::unitRef,  make_tuple(L"unit reference", &LinkageSegment::decode_reference) },
		{ LinkageType::globRef,  make_tuple(L"global reference", &LinkageSegment::decode_reference) },
		{ LinkageType::publRef,  make_tuple(L"public reference", &LinkageSegment::decode_reference) },
		{ LinkageType::privRef,  make_tuple(L"private reference", &LinkageSegment::decode_privateReference) },
		{ LinkageType::constRef, make_tuple(L"constant reference", &LinkageSegment::decode_reference) },
		{ LinkageType::globDef,  make_tuple(L"global definition", &LinkageSegment::decode_globalDefinition) },
		{ LinkageType::publDef,  make_tuple(L"public definition", &LinkageSegment::decode_publicDefinition) },
		{ LinkageType::constDef, make_tuple(L"constant value", &LinkageSegment::decode_constantDefinition) },
		{ LinkageType::extProc,  make_tuple(L"external procedure", &LinkageSegment::decode_externalRoutine) },
		{ LinkageType::extFunc,  make_tuple(L"external function", &LinkageSegment::decode_externalRoutine) },
		{ LinkageType::sepProc,  make_tuple(L"separate procedure", &LinkageSegment::decode_externalRoutine) },
		{ LinkageType::sepFunc,  make_tuple(L"separate function", &LinkageSegment::decode_externalRoutine) },
	};

}