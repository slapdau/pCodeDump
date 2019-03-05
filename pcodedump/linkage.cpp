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

#include "linkage.hpp"
#include "segment.hpp"
#include <iostream>
#include <string>
#include <map>
#include <iomanip>
#include <boost/endian/arithmetic.hpp>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	LinkageInfo::LinkageInfo(Segment & segment, const std::uint8_t * linkage) :
		segment{ segment }, linkage{ linkage }
	{
	}

	/* Write out linkage records. */
	void LinkageInfo::write(std::wostream & os) const
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
	uint8_t const * LinkageInfo::decode_referenceArray(wostream & os, uint8_t const *current, int numberOfRefs) const {
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

	uint8_t const * LinkageInfo::decode_reference(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << static_cast<OperandFormat>(static_cast<int>(parameters[0])) << endl;
		return decode_referenceArray(os, current + sizeof(little_int16_t) * 3, parameters[1]);
	}

	uint8_t const * LinkageInfo::decode_privateReference(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << static_cast<OperandFormat>(static_cast<int>(parameters[0])) << L" (" << parameters[2] << L" words)" << endl;
		return decode_referenceArray(os, current + sizeof(little_int16_t) * 3, parameters[1]);
	}

	uint8_t const * LinkageInfo::decode_globalDefinition(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"#" << parameters[0] << L", IC=" << parameters[1] << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageInfo::decode_publicDefinition(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"base = " << parameters[0] << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageInfo::decode_constantDefinition(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"= " << parameters[0] << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageInfo::decode_externalRoutine(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		os << dec << L"#" << parameters[0] << " (" << parameters[1] << L" words)" << endl;
		return current + sizeof(little_int16_t) * 3;
	}

	uint8_t const * LinkageInfo::decode_endOfFile(wostream & os, uint8_t const *current) const {
		auto parameters = reinterpret_cast<little_int16_t const *>(current);
		if (segment.getSegmentKind() != SegmentKind::seprtseg) {
			os << dec << parameters[0] << " global words";
			if (segment.getSegmentKind() == SegmentKind::unlinkedIntrins) {
				os << ", private data seg #" << parameters[1];
			}
			os << endl;
		}
		return nullptr;
	}

	map<LinkageType, tuple<wstring, LinkageInfo::decode_function_t>> LinkageInfo::linkageNames = {
		{ LinkageType::eofMark,  make_tuple(L"end of linkage", &LinkageInfo::decode_endOfFile) },
		{ LinkageType::unitRef,  make_tuple(L"unit reference", &LinkageInfo::decode_reference) },
		{ LinkageType::globRef,  make_tuple(L"global reference", &LinkageInfo::decode_reference) },
		{ LinkageType::publRef,  make_tuple(L"public reference", &LinkageInfo::decode_reference) },
		{ LinkageType::privRef,  make_tuple(L"private reference", &LinkageInfo::decode_privateReference) },
		{ LinkageType::constRef, make_tuple(L"constant reference", &LinkageInfo::decode_reference) },
		{ LinkageType::globDef,  make_tuple(L"global definition", &LinkageInfo::decode_globalDefinition) },
		{ LinkageType::publDef,  make_tuple(L"public definition", &LinkageInfo::decode_publicDefinition) },
		{ LinkageType::constDef, make_tuple(L"constant value", &LinkageInfo::decode_constantDefinition) },
		{ LinkageType::extProc,  make_tuple(L"external procedure", &LinkageInfo::decode_externalRoutine) },
		{ LinkageType::extFunc,  make_tuple(L"external function", &LinkageInfo::decode_externalRoutine) },
		{ LinkageType::sepProc,  make_tuple(L"separate procedure", &LinkageInfo::decode_externalRoutine) },
		{ LinkageType::sepFunc,  make_tuple(L"separate function", &LinkageInfo::decode_externalRoutine) },
	};

}
