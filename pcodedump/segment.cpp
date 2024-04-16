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

#include "segment.hpp"
#include "linkage.hpp"
#include "basecode.hpp"
#include "text.hpp"
#include "textio.hpp"
#include "options.hpp"
#include <map>
#include <cassert>
#include <stdexcept>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	uint64_t SegmentDictionary::intrinsicSegments() const {
		return intrinsicSegs;
	}

	std::wstring SegmentDictionary::fileComment() const {
		int size = comment[0];
		return wstring{ comment + 1, comment + 1 + size };
	}

	SegmentDictionaryEntry const SegmentDictionary::operator[](int index) const {
		return SegmentDictionaryEntry{ this, index };
	}

	SegmentDictionaryEntry::SegmentDictionaryEntry(SegmentDictionary const * segmentDictionary, int index) :
		segmentDictionary{ segmentDictionary }, index{ index }
	{
		if (0 > index || index >= SegmentDictionary::NUM_SEGMENTS) {
			throw out_of_range("Segment dictionary index out of bounds: " + index);
		}
	}

	int SegmentDictionaryEntry::codeAddress() const {
		return segmentDictionary->diskInfo[index].codeaddr;
	}

	int SegmentDictionaryEntry::codeLength() const {
		return segmentDictionary->diskInfo[index].codeleng;
	}

	std::wstring SegmentDictionaryEntry::name() const {
		return wstring{ segmentDictionary->segName[index], segmentDictionary->segName[index] + 8 };
	}

	int SegmentDictionaryEntry::textAddress() const {
		return segmentDictionary->textAddr[index];
	}

	SegmentKind SegmentDictionaryEntry::segmentKind() const {
		return static_cast<SegmentKind>(int{ segmentDictionary->segKind[index] });
	}

	int SegmentDictionaryEntry::segmentNumber() const {
		return int{ segmentDictionary->segInfo[index] } &0xff;
	}

	MachineType SegmentDictionaryEntry::machineType() const {
		return static_cast<MachineType>(int{ segmentDictionary->segInfo[index] } >> 8 & 0xf);
	}

	int SegmentDictionaryEntry::version() const {
		return int{ segmentDictionary->segInfo[index] } >> 13 & 0x7;
	}

	int SegmentDictionaryEntry::startAddress() const {
		return textAddress() ? textAddress() : codeAddress();
	}

	int SegmentDictionaryEntry::linkageAddress() const {
		return codeAddress() + codeLength() / BLOCK_SIZE + 1;
	}

	map<SegmentKind, wstring> segKind = {
		{SegmentKind::linked,          L"LINKED"},
		{SegmentKind::hostseg,         L"HOSTSEG"},
		{SegmentKind::segproc,         L"SEGPROC"},
		{SegmentKind::unitseg,         L"UNITSEG"},
		{SegmentKind::seprtseg,        L"SEPRTSEG"},
		{SegmentKind::unlinkedIntrins, L"UNLINKED-INTRINS"},
		{SegmentKind::linkedIntrins,   L"LINKED-INTRINS"},
		{SegmentKind::dataSeg,         L"DATASEG"},
	};

	std::wostream& operator<<(std::wostream& os, const SegmentKind& value) {
		os << segKind[value];
		return os;
	}

	map<MachineType, wstring> machineType = {
		{MachineType::undentified,    L"Unidentified"},
		{MachineType::pcode_big,      L"P-Code (MSB)"},
		{MachineType::pcode_little,   L"P-Code (LSB)"},
		{MachineType::native_pdp11,   L"Native (PDP-11)"},
		{MachineType::native_m8080,   L"Native (8080)"},
		{MachineType::native_z80,     L"Native (Z80)"},
		{MachineType::native_ga440,   L"Native (GA 440)"},
		{MachineType::native_m6502,   L"Native (6502)"},
		{MachineType::native_m6800,   L"Native (6800)"},
		{MachineType::native_tms9900, L"Native (TMS9900)"},
	};

	std::wostream& operator<<(std::wostream& os, const MachineType& value) {
		os << machineType[value];
		return os;
	}

	Segment::Segment(SegmentDictionaryEntry const dictionaryEntry) :
		dictionaryEntry{ dictionaryEntry }
	{
	}

	Segment::~Segment() {
	}

	std::wostream& Segment::writeOut(std::wostream& os) const
	{
		FmtSentry<wostream::char_type> sentry{ os };

		os << "Segment " << dec << dictionaryEntry.segmentNumber() << L": ";
		os << dictionaryEntry.name() << L" (" << dictionaryEntry.segmentKind() << L")" << endl;
		os << L"  Segment info : version=" << dictionaryEntry.version() << ", mType=" << dictionaryEntry.machineType() << endl;
		os << L"        Length : " << dictionaryEntry.codeLength() << endl;
		return os;
	}

	DataSegment::DataSegment(SegmentDictionaryEntry const dictionaryEntry) :
		Segment{ dictionaryEntry }
	{
	}

	std::wostream& operator<<(std::wostream& os, const Segment & segment) {
		return segment.writeOut(os);
	}

	bool CodeSegment::showText = false;
	bool CodeSegment::listProcs = false;
	bool CodeSegment::showLinkage = false;
	
	CodeSegment::CodeSegment(buff_t const & buffer, SegmentDictionaryEntry const dictionaryEntry, int endBlock) :
		Segment{ dictionaryEntry},
		buffer{ buffer },
		endBlock{ endBlock },
		codePart{ createCodePart() },
		interfaceText{ createInterfaceText() },
		linkageInfo{ createLinkageInfo() }
	{
	}

	std::wostream& CodeSegment::writeOut(std::wostream& os) const {
		writeHeader(os);
		os << endl;
		if (showText && interfaceText) {
			interfaceText->write(os);
			os << endl;
		}
		if (listProcs && codePart) {
			codePart->disassemble(os, linkageInfo.get());
			os << endl;
		}
		if (showLinkage && linkageInfo) {
			linkageInfo->write(os);
			os << endl;
		}
		return os;
	}

	unique_ptr<CodePart> CodeSegment::createCodePart() {
		assert(dictionaryEntry.codeAddress());
		return make_unique<CodePart>(*this, buffer.data() + dictionaryEntry.codeAddress() * BLOCK_SIZE, dictionaryEntry.codeLength());
	}

	/* Create a new interface text segment if this directry entry points to one. */
	unique_ptr<InterfaceText> CodeSegment::createInterfaceText() {
		if (dictionaryEntry.textAddress()) {
			return make_unique<InterfaceText>(
				*this,
				buffer.data() + dictionaryEntry.textAddress() * BLOCK_SIZE,
				buffer.data() + dictionaryEntry.codeAddress() * BLOCK_SIZE
			);
		} else {
			return unique_ptr<InterfaceText>();
		}
	}

	/* Create a new linkage segment if this directory entry has unlinked code.
	   The location of linkage data has to be inferred as the block following code data. */
	unique_ptr<LinkageInfo> CodeSegment::createLinkageInfo()
	{
		if (dictionaryEntry.linkageAddress() != this->endBlock) {
			return make_unique<LinkageInfo>(*this, buffer.data() + dictionaryEntry.linkageAddress() * BLOCK_SIZE);
		} else {
			return unique_ptr<LinkageInfo>();
		}
	}

	void CodeSegment::writeHeader(std::wostream& os) const {
		Segment::writeOut(os);

		FmtSentry<wostream::char_type> sentry{ os };
		os << L"   Text blocks : ";
		if (dictionaryEntry.textAddress()) {
			os << dictionaryEntry.textAddress() << L" - " << dictionaryEntry.codeAddress() - 1 << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"   Code blocks : ";
		if (dictionaryEntry.codeAddress()) {
			os << dictionaryEntry.codeAddress() << L" - " << dictionaryEntry.linkageAddress() - 1 << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"   Link blocks : ";
		if (dictionaryEntry.linkageAddress() != this->endBlock) {
			os << dictionaryEntry.linkageAddress() << L" - " << this->endBlock - 1 << endl;
		} else {
			os << L"-----" << endl;
		}
		if (this->codePart.get() != nullptr) {
			this->codePart->writeHeader(os);
		}
	}

}