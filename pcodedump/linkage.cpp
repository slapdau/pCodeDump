/*
   Copyright 2017-2023 Craig McGeachie

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
#include <memory>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	namespace {

		map<LinkageType, wstring> linkageNames = {
			{ LinkageType::eofMark,  L"end of linkage" },
			{ LinkageType::unitRef,  L"unit reference" },
			{ LinkageType::globRef,  L"global reference" },
			{ LinkageType::publRef,  L"public reference" },
			{ LinkageType::privRef,  L"private reference" },
			{ LinkageType::constRef, L"constant reference" },
			{ LinkageType::globDef,  L"global definition" },
			{ LinkageType::publDef,  L"public definition" },
			{ LinkageType::constDef, L"constant value" },
			{ LinkageType::extProc,  L"external procedure" },
			{ LinkageType::extFunc,  L"external function" },
			{ LinkageType::sepProc,  L"separate procedure" },
			{ LinkageType::sepFunc,  L"separate function" },
		};

		std::wostream& operator<<(std::wostream& os, const LinkageType& value) {
			os << linkageNames[value];
			return os;
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

	template <typename T>
	T const & place(std::uint8_t const * address) {
		return *reinterpret_cast<T const *>(address);
	}

	LinkRecord::LinkRecord(std::wstring name, std::uint8_t const * fieldStart) : name{ name }, fieldStart{ fieldStart }
	{
	}

	LinkRecord::~LinkRecord() = default;

	std::uint8_t const * LinkRecord::end() const
	{
		return fieldStart + sizeof(little_int16_t[3]);
	}

	void LinkRecord::writeOut(std::wostream & os) const
	{
		os << L"  " << name << L" " << setfill(L' ') << left << setw(20) << linkRecordType() << L" ";
	}

	std::wstring LinkRecord::getName() const
	{
		return name;
	}

	struct LinkReference::Fields {
		boost::endian::little_int16_t format;
		boost::endian::little_int16_t numberOfReferences;
		boost::endian::little_int16_t numberOfWords;
		boost::endian::little_int16_t references;
	};

	LinkReference::LinkReference(std::wstring name, std::uint8_t const * fieldStart) :
		LinkRecord{ name, fieldStart },
		fields{ place<Fields>(fieldStart) },
		references{ extractReferences() }
	{
	}

	LinkReference::~LinkReference() = default;

	std::uint8_t const * LinkReference::end() const
	{
		intptr_t numberOfReferences = fields.numberOfReferences;
		auto references = reinterpret_cast<int16_t const *>(fields.references.data());
		return reinterpret_cast<uint8_t const *>(references + ((numberOfReferences + 7) / 8) * 8);
	}

	void LinkReference::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		os << static_cast<OperandFormat>(fields.format.value());
		writeReferences(os);
		os << endl;
	}

	void LinkReference::writeReferences(std::wostream & os) const
	{
		os << hex << setfill(L'0') << right;
		int count = 0;
		for (auto reference : references) {
			if (count % 8 == 0) {
				os << endl << "    ";
			}
			os << setw(4) << reference << L" ";
			++count;
		}
	}

	std::vector<int> const LinkReference::getReferences() const
	{
		return references;
	}

	std::vector<int> LinkReference::extractReferences()
	{
		auto references = &fields.references;
		std::vector<int> result;
		for (int index = 0; index != fields.numberOfReferences; ++index) {
			result.push_back(references[index]);
		}
		return result;
	}

	void PrivateReference::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		os << static_cast<OperandFormat>(fields.format.value()) << L" (" << fields.numberOfWords << L" words)";
		writeReferences(os);
		os << endl;
	}

	class ConstantReference final : public LinkReference {
	public:
		ConstantReference(std::wstring name, std::uint8_t const * fieldStart) : LinkReference(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::constRef; }
	};

	class UnitReference final : public LinkReference {
	public:
		UnitReference(std::wstring name, std::uint8_t const * fieldStart) : LinkReference(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::unitRef; }
	};

	struct GlobalDefinition::Fields {
		boost::endian::little_int16_t homeProcedure;
		boost::endian::little_int16_t icOffset;
	};

	GlobalDefinition::GlobalDefinition(std::wstring name, std::uint8_t const * fieldStart) :
		LinkRecord{ name, fieldStart },
		fields{ place<Fields>(fieldStart) }
	{
	}

	void GlobalDefinition::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		os << dec << L"#" << fields.homeProcedure << L", IC=" << fields.icOffset << endl;
	}

	struct PublicDefinition::Fields {
		boost::endian::little_int16_t baseOffset;
	};

	PublicDefinition::PublicDefinition(std::wstring name, std::uint8_t const * fieldStart) :
		LinkRecord{ name, fieldStart },
		fields{ place<Fields>(fieldStart) }
	{
	}

	void PublicDefinition::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		os << dec << L"base = " << fields.baseOffset << endl;
	}

	struct ConstantDefinition::Fields {
		boost::endian::little_int16_t constantValue;
	};

	ConstantDefinition::ConstantDefinition(std::wstring name, std::uint8_t const * fieldStart) :
		LinkRecord{ name, fieldStart },
		fields{ place<Fields>(fieldStart) }
	{
	}

	void ConstantDefinition::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		os << dec << L"= " << fields.constantValue << endl;
	}

	struct LinkRoutine::Fields {
		boost::endian::little_int16_t sourceProcedure;
		boost::endian::little_int16_t numberOfParams;
	};

	LinkRoutine::LinkRoutine(std::wstring name, std::uint8_t const * fieldStart) :
		LinkRecord{ name, fieldStart },
		fields{ place<Fields>(fieldStart) }
	{
	}
	LinkRoutine::~LinkRoutine() = default;

	void LinkRoutine::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		os << dec << L"#" << fields.sourceProcedure << " (" << fields.numberOfParams << L" words)" << endl;
	}

	struct EndOfFileMark::Fields {
		boost::endian::little_int16_t nextBaseLc;
		boost::endian::little_int16_t privateDataSegment;
	};

	EndOfFileMark::EndOfFileMark(std::wstring name, SegmentKind const segmentKind, std::uint8_t const * fieldStart) :
		LinkRecord{ name, fieldStart },
		fields{ place<Fields>(fieldStart) },
		segmentKind{ segmentKind }
	{
	}

	void EndOfFileMark::writeOut(std::wostream & os) const
	{
		LinkRecord::writeOut(os);
		if (segmentKind != SegmentKind::seprtseg) {
			os << dec << fields.nextBaseLc << " global words";
			if (segmentKind == SegmentKind::unlinkedIntrins) {
				os << ", private data seg #" << fields.privateDataSegment;
			}
			os << endl;
		}
	}

	shared_ptr<LinkRecord const> readLinkRecord(CodeSegment & segment, uint8_t const * address) {
		struct Header {
			char name[8];
			little_int16_t linkRecordType;
			uint8_t fieldsStart;
		};
		Header const & header{ place<Header>(address) };
		wstring name{ cbegin(header.name), cend(header.name) };
		auto linkageType = static_cast<LinkageType>(header.linkRecordType.value());
		switch (linkageType) {
		case LinkageType::eofMark:
			return make_shared<EndOfFileMark>(name, segment.getSegmentKind(), &header.fieldsStart);
		case LinkageType::unitRef:
			return make_shared<UnitReference>(name, &header.fieldsStart);
		case LinkageType::globRef:
			return make_shared<GlobalReference>(name, &header.fieldsStart);
		case LinkageType::publRef:
			return make_shared<PublicReference>(name, &header.fieldsStart);
		case LinkageType::privRef:
			return make_shared<PrivateReference>(name, &header.fieldsStart);
		case LinkageType::constRef:
			return make_shared<ConstantReference>(name, &header.fieldsStart);
		case LinkageType::globDef:
			return make_shared<GlobalDefinition>(name, &header.fieldsStart);
		case LinkageType::publDef:
			return make_shared<PublicDefinition>(name, &header.fieldsStart);
		case LinkageType::constDef:
			return make_shared<ConstantDefinition>(name, &header.fieldsStart);
		case LinkageType::extProc:
			return make_shared<ExternalProcedure>(name, &header.fieldsStart);
		case LinkageType::extFunc:
			return make_shared<ExternalFunction>(name, &header.fieldsStart);
		case LinkageType::sepProc:
			return make_shared<SeparateProcedure>(name, &header.fieldsStart);
		case LinkageType::sepFunc:
			return make_shared<SeparateFunction>(name, &header.fieldsStart);
		default:
			return shared_ptr<LinkRecord const>();
		}
	}

	vector<shared_ptr<LinkRecord const>> readLinkRecords(CodeSegment & segment, uint8_t const * linkageBase) {
		vector<shared_ptr<LinkRecord const>> result;
		uint8_t const * currentBase = linkageBase;
		do {
			result.push_back(readLinkRecord(segment, currentBase));
			currentBase = result.back()->end();
		} while (!result.back()->endOfLinkage());
		return result;
	}

	std::wostream& operator<<(std::wostream& os, LinkRecord const & record) {
		record.writeOut(os);
		return os;
	}

	LinkageInfo::LinkageInfo(CodeSegment & segment, const std::uint8_t * linkageBase) :
		linkRecords{ readLinkRecords(segment, linkageBase) }
	{
	}


	/* Write out linkage records. */
	void LinkageInfo::write(std::wostream & os) const
	{
		os << L"Linkage records:" << endl;
		for (auto & record : linkRecords) {
			os << *record;
		}
	}

	std::vector<std::shared_ptr<LinkRecord const>> LinkageInfo::getLinkRecords() const
	{
		return linkRecords;
	}
}
