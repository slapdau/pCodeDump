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

#include "directory.hpp"
#include "pcode.hpp"
#include "text.hpp"
#include "linkage.hpp"
#include "basecode.hpp"
#include "native6502.hpp"
#include "textio.hpp"
#include "options.hpp"

#include <iomanip>
#include <iterator>
#include <algorithm>
#include <tuple>
#include <vector>
#include <set>
#include <memory>
#include <algorithm>
#include <cassert>
#include <map>
#include <functional>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	class SegmentDictionary {
	public:
		friend class SegmentDictionaryEntry;

		uint64_t intrinsicSegments() const;
		std::wstring fileComment() const;

		SegmentDictionaryEntry const operator[](int index) const;

	private:
		struct {
			boost::endian::little_int16_t codeaddr;
			boost::endian::little_int16_t codeleng;
		} diskInfo[16];
		char segName[16][8];
		boost::endian::little_int16_t segKind[16];
		boost::endian::little_int16_t textAddr[16];
		boost::endian::little_int16_t segInfo[16];
		boost::endian::little_uint64_t intrinsicSegs;
		boost::endian::little_uint16_t filler[68];
		char comment[80];
	};

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
	{}

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

	Segment::Segment(buff_t const & buffer, SegmentDictionaryEntry const dictionaryEntry, int endBlock) :
		buffer{ buffer },
		dictionaryEntry{ dictionaryEntry },
		nextSegBlock{ endBlock },
		codePart{ createCodePart() },
		interfaceText{ createInterfaceText() },
		linkageInfo{ createLinkageInfo() }
	{
	}

	unique_ptr<CodePart> Segment::createCodePart() {
		assert(dictionaryEntry.codeAddress());
		return make_unique<CodePart>(*this, buffer.data() + dictionaryEntry.codeAddress() * BLOCK_SIZE, dictionaryEntry.codeLength());
	}

	/* Create a new interface text segment if this directry entry points to one. */
	unique_ptr<InterfaceText> Segment::createInterfaceText() {
		if (dictionaryEntry.textAddress()) {
			return make_unique<InterfaceText>(*this, buffer.data() + dictionaryEntry.textAddress() * BLOCK_SIZE);
		} else {
			return unique_ptr<InterfaceText>(nullptr);
		}
	}

	namespace {

		/* Predicate function to determine if a particular type of segment has linkage information.
		   This will be segments output by the compiler or assembler that either export symbols
		   or have unresolved symbolic references. */
		bool hasLinkage(SegmentKind segmentKind) {
			static set<SegmentKind> LINKAGE_SEGMENTS = {
				SegmentKind::hostseg,
				SegmentKind::unitseg,
				SegmentKind::unlinkedIntrins,
				SegmentKind::seprtseg,
			};
			return LINKAGE_SEGMENTS.find(segmentKind) != end(LINKAGE_SEGMENTS);
		}

	}

	/* Create a new linkage segment if this directory entry has unlinked code.
	   The location of linkage data has to be inferred as the block following code data. */
	unique_ptr<LinkageInfo> Segment::createLinkageInfo()
	{
		if (hasLinkage(getSegmentKind())) {
			assert(dictionaryEntry.codeAddress() + dictionaryEntry.codeLength() / BLOCK_SIZE + 1 != static_cast<unsigned int>(this->nextSegBlock));
			return make_unique<LinkageInfo>(*this, buffer.data() + (dictionaryEntry.codeAddress() + dictionaryEntry.codeLength() / BLOCK_SIZE + 1) * BLOCK_SIZE);
		} else {
			return unique_ptr<LinkageInfo>(nullptr);
		}
	}

	void Segment::writeHeader(std::wostream& os) const {
		FmtSentry<wostream::char_type> sentry{ os };
		os << "Segment " << dec << dictionaryEntry.segmentNumber() << L": ";
		os << dictionaryEntry.name() << L" (" << dictionaryEntry.segmentKind() << L")" << endl;
		os << L"   Text blocks : ";
		if (dictionaryEntry.textAddress()) {
			os << dictionaryEntry.textAddress() << L" - " << dictionaryEntry.codeAddress() - 1 << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"   Code blocks : ";
		if (dictionaryEntry.codeAddress()) {
			os << dictionaryEntry.codeAddress() << L" - " << (dictionaryEntry.codeAddress() + dictionaryEntry.codeLength() / BLOCK_SIZE) << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"   Link blocks : ";
		if (dictionaryEntry.codeAddress() && dictionaryEntry.codeAddress() + dictionaryEntry.codeLength() / BLOCK_SIZE + 1 != static_cast<unsigned int>(this->nextSegBlock)) {
			os << (dictionaryEntry.codeAddress() + dictionaryEntry.codeLength() / BLOCK_SIZE + 1) << L" - " << (this->nextSegBlock - 1) << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"        Length : " << dictionaryEntry.codeLength() << endl;
		os << L"  Segment info : version=" << dictionaryEntry.version() << ", mType=" << dictionaryEntry.machineType() << endl;
		if (this->codePart.get() != nullptr) {
			this->codePart->writeHeader(os);
		}
	}

	std::wostream& operator<<(std::wostream& os, const Segment & segment) {
		segment.writeHeader(os);
		os << endl;
		if (showText && segment.interfaceText.get() != nullptr) {
			segment.interfaceText->write(os);
			os << endl;
		}
		if (listProcs && segment.codePart.get() != nullptr) {
			segment.codePart->disassemble(os);
			os << endl;
		}
		if (showLinkage && segment.linkageInfo.get() != nullptr) {
			segment.linkageInfo->write(os);
			os << endl;
		}
		return os;
	}

	PcodeFile::PcodeFile(buff_t const & buffer) :
		buffer{ buffer },
		segmentDictionary{ *reinterpret_cast<SegmentDictionary const *>(buffer.data()) },
		segments{ extractSegments() }
	{
	}


	bool reverseAddress(SegmentDictionaryEntry const & left, SegmentDictionaryEntry const & right) {
		return left.startAddress() > right.startAddress();
	}

	bool index(shared_ptr<Segment const> left, shared_ptr<Segment const> right) {
		return left->getDictionaryIndex() > right->getDictionaryIndex();
	}

	/* Scan the directory and return a list of segments, and
	   and, if the segment has blocks of information in the file (everything except data segments),
	   also return the segment end.

	   The main reason for doing this is that the directory entry objects need to be able to write
	   block ranges for linkage information, but these block ranges are inferred from the gaps
	   between the end of code in one segment and the start of the next segment.  Directory entries
	   are self contained and don't refer to each other.  So the inferred endings for each segment
	   are calculated and passed to directory entry constructors.

	   Data blocks use 0 as a special value for the segment end. Segment block ranges are treated
	   as [begin, end), so the block number returned is actually one block past the end block of
	   the segment.  For the last segment in a file, this block number be past the end of the file. */
	unique_ptr<Segments> PcodeFile::extractSegments() {

		vector<SegmentDictionaryEntry> dictionaryEntries;
		for (int directoryIndex = 0; directoryIndex != 16; ++directoryIndex) {
			if (segmentDictionary[directoryIndex].codeAddress() != 0) {
				dictionaryEntries.push_back(segmentDictionary[directoryIndex]);
			}
		}

		sort(begin(dictionaryEntries), end(dictionaryEntries), reverseAddress);

		auto segments = make_unique<Segments>();
		int currentEnd = static_cast<int>(((buffer.size() - 1) / BLOCK_SIZE + 1));
		for (auto dictionaryEntry : dictionaryEntries) {
			segments->push_back(make_shared<Segment>(buffer, dictionaryEntry, currentEnd));
			currentEnd = dictionaryEntry.startAddress();
		}

		sort(begin(*segments), end(*segments), index);

		return segments;
	}

	namespace {

		/* Take a 64 bit value of flags, representing intrinsic units used, and write out a sequence
		   of integers for the bit positions that have a value of 1. */
		void writeIntrinsicUnits(std::wostream& os, uint64_t value) {
			os << L"Intrinsic units required: ";
			if (value == 0) {
				os << L"None";
			} else {
				wstring sep = L"";
				for (int count = 0; count != 64; ++count) {
					if (value & 0x1) {
						os << sep << count;
						sep = L"  ";
					}
					value >>= 1;
				}
			}
			os << endl;
		}
	}

	std::wostream& operator<<(std::wostream& os, const PcodeFile& file) {
		FmtSentry<wostream::char_type> sentry{ os };
		wcout << "Total blocks: " << (file.buffer.size() - 1) / BLOCK_SIZE + 1 << endl;
		wstring comment = file.segmentDictionary.fileComment();
		transform(begin(comment), end(comment), begin(comment), [](const auto &c) { return 32 <= c && c <= 126 ? c : L'.'; });
		os << L"Comment: " << comment << endl;
		writeIntrinsicUnits(os, file.segmentDictionary.intrinsicSegments());
		os << endl;
		Segments segments;
		copy(begin(*file.segments), end(*file.segments), back_inserter(segments));
		if (addressOrder) {
			sort(begin(segments), end(segments), [](const auto &left, const auto &right) { return left->getFirstBlock() < right->getFirstBlock(); });
		}
		for (auto segment : segments) {
			os << *segment << endl;
		}
		return os;
	}
}
