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

using namespace std;
using namespace boost::endian;

namespace pcodedump {

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

	SegmentDirectoryEntry::SegmentDirectoryEntry(buff_t const & buffer, SegmentDictionary const * segmentDictionary, int index, int endBlock) :
		buffer{ buffer },
		name{ segmentDictionary->segName[index], segmentDictionary->segName[index] + 8 },
		textBlock{ segmentDictionary->textAddr[index] },
		codeBlock{ segmentDictionary->diskInfo[index].codeaddr },
		codeLength{ segmentDictionary->diskInfo[index].codeleng },
		nextSegBlock{ endBlock },
		segmentKind{ static_cast<SegmentKind>(int{ segmentDictionary->segKind[index] }) },
		segmentNumber{ int{ segmentDictionary->segInfo[index] } &0xff },
		machineType{ static_cast<MachineType>(int{ segmentDictionary->segInfo[index] } >> 8 & 0xf) },
		version{ int{ segmentDictionary->segInfo[index] } >> 13 & 0x7 },
		codePart{ createCodePart() },
		interfaceText{ createInterfaceText() },
		linkageInfo{ createLinkageInfo() }
	{
	}

	/* Create a new correctly subtyped code segment object if this directory entry has code (everything except data segments). */
	unique_ptr<CodePart> SegmentDirectoryEntry::createCodePart() {
		if (hasPcode()) {
			return make_unique<CodePart>(*this, buffer.data() + codeBlock * BLOCK_SIZE, codeLength);
		} else if (has6502code()) {
			return make_unique<CodePart>(*this, buffer.data() + codeBlock * BLOCK_SIZE, codeLength);
		} else {
			return unique_ptr<CodePart>(nullptr);
		}
	}

	/* Create a new interface text segment if this directry entry points to one. */
	unique_ptr<InterfaceText> SegmentDirectoryEntry::createInterfaceText() {
		if (this->textBlock) {
			return make_unique<InterfaceText>(*this, buffer.data() + textBlock * BLOCK_SIZE);
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
	unique_ptr<LinkageInfo> SegmentDirectoryEntry::createLinkageInfo()
	{
		if (hasLinkage(this->segmentKind)) {
			assert(this->codeBlock + this->codeLength / BLOCK_SIZE + 1 != static_cast<unsigned int>(this->nextSegBlock));
			return make_unique<LinkageInfo>(*this, this->buffer.data() + (this->codeBlock + this->codeLength / BLOCK_SIZE + 1) * BLOCK_SIZE);
		} else {
			return unique_ptr<LinkageInfo>(nullptr);
		}
	}

	void SegmentDirectoryEntry::writeHeader(std::wostream& os) const {
		FmtSentry<wostream::char_type> sentry{ os };
		os << "Segment " << dec << this->segmentNumber << L": ";
		os << this->name << L" (" << this->segmentKind << L")" << endl;
		os << L"   Text blocks : ";
		if (this->textBlock) {
			os << this->textBlock << L" - " << this->codeBlock - 1 << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"   Code blocks : ";
		if (this->codeBlock) {
			os << this->codeBlock << L" - " << (this->codeBlock + this->codeLength / BLOCK_SIZE) << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"   Link blocks : ";
		if (this->codeBlock && this->codeBlock + this->codeLength / BLOCK_SIZE + 1 != static_cast<unsigned int>(this->nextSegBlock)) {
			os << (this->codeBlock + this->codeLength / BLOCK_SIZE + 1) << L" - " << (this->nextSegBlock - 1) << endl;
		} else {
			os << L"-----" << endl;
		}
		os << L"        Length : " << this->codeLength << endl;
		os << L"  Segment info : version=" << this->version << ", mType=" << this->machineType << endl;
		if (this->codePart.get() != nullptr) {
			this->codePart->writeHeader(os);
		}
	}

	std::wostream& operator<<(std::wostream& os, const SegmentDirectoryEntry& value) {
		value.writeHeader(os);
		os << endl;
		if (showText && value.interfaceText.get() != nullptr) {
			value.interfaceText->write(os);
			os << endl;
		}
		if (listProcs && value.codePart.get() != nullptr) {
			value.codePart->disassemble(os);
			os << endl;
		}
		if (showLinkage && value.linkageInfo.get() != nullptr) {
			value.linkageInfo->write(os);
			os << endl;
		}
		return os;
	}

	SegmentDirectory::SegmentDirectory(buff_t const & buffer) :
		buffer{ buffer },
		segmentDictionary{ reinterpret_cast<SegmentDictionary const *>(buffer.data()) },
		entries{ extractDirectoryEntries() },
		intrinsicLibraries{ segmentDictionary->intrinsicSegs }
	{
		int size = segmentDictionary->comment[0];
		comment = wstring{ segmentDictionary->comment + 1, segmentDictionary->comment + 1 + size };
	}

	namespace {

		/* Scan the directory and return a list of directory entry indexes that define segments, and
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
		auto getSegmentEnds(buff_t const & buffer) {
			auto segmentDictionary = reinterpret_cast<SegmentDictionary const *>(buffer.data());
			vector<tuple<int, int>> segmentStarts;
			for (int directoryIndex = 0; directoryIndex != 16; ++directoryIndex) {
				if (segmentDictionary->diskInfo[directoryIndex].codeleng) {
					int codeaddr = segmentDictionary->diskInfo[directoryIndex].codeaddr;
					int textaddr = segmentDictionary->textAddr[directoryIndex];
					segmentStarts.push_back(make_tuple(directoryIndex, textaddr != 0 ? textaddr : codeaddr));
				}
			}
			// Sort by segment starts.  Data segments, which have no space allocated in file, have a start
			// of 0 and are conveniently sorted to the start of vector out of the way.
			sort(begin(segmentStarts), end(segmentStarts), [](const auto &left, const auto &right) { return get<1>(left) < get<1>(right); });
			vector<tuple<int, int>> result;
			for (unsigned int index = 0; index != segmentStarts.size() - 1; ++index) {
				int directoryIndex, segmentStart;
				tie(directoryIndex, segmentStart) = segmentStarts[index];
				// If the segment start is zero it's a data segment.  Retain that information with a 0 for the end.
				if (segmentStart) {
					int nextSegmentStart;
					tie(ignore, nextSegmentStart) = segmentStarts[index + 1];
					result.push_back(make_tuple(directoryIndex, nextSegmentStart));
				} else {
					result.push_back(make_tuple(directoryIndex, 0));
				}
			}
			{
				int directoryIndex, segmentStart;
				tie(directoryIndex, segmentStart) = segmentStarts[segmentStarts.size() - 1];
				int end = static_cast<int>(((buffer.size() - 1) / BLOCK_SIZE + 1));
				result.push_back(make_tuple(directoryIndex, end));
			}
			// Sort by index number to restore the order in the segment directory.
			sort(begin(result), end(result), [](const auto &left, const auto &right) { return get<0>(left) < get<0>(right); });
			return result;
		}
	}

	/* Create a list of directory entries for defined segments in the order they are in the directory.  Unused
	   directory entry slots will not be returned. */
	unique_ptr<SegmentEntries> SegmentDirectory::extractDirectoryEntries() {
		auto segmentDictionary = reinterpret_cast<SegmentDictionary const *>(buffer.data());
		auto entries = make_unique<SegmentEntries>();
		for (auto segment : getSegmentEnds(buffer)) {
			auto [directoryIndex, segmentEnd] = segment;
			entries->push_back(make_shared<SegmentDirectoryEntry>(buffer, segmentDictionary, directoryIndex, segmentEnd));
		}
		return entries;
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

	std::wostream& operator<<(std::wostream& os, const SegmentDirectory& value) {
		FmtSentry<wostream::char_type> sentry{ os };
		wcout << "Total blocks: " << (value.buffer.size() - 1) / BLOCK_SIZE + 1 << endl;
		wstring comment = value.comment;
		transform(begin(comment), end(comment), begin(comment), [](const auto &c) { return 32 <= c && c <= 126 ? c : L'.'; });
		os << L"Comment: " << comment << endl;
		writeIntrinsicUnits(os, value.intrinsicLibraries);
		os << endl;
		SegmentEntries entries;
		copy(begin(*value.entries), end(*value.entries), back_inserter(entries));
		if (addressOrder) {
			sort(begin(entries), end(entries), [](const auto &left, const auto &right) { return left->getFirstBlock() < right->getFirstBlock(); });
		}
		for (auto entry : entries) {
			os << *entry << endl;
		}
		return os;
	}
}
