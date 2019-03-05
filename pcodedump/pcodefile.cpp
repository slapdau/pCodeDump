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

#include "pcodefile.hpp"
#include "segment.hpp"
#include "options.hpp"
#include "textio.hpp"

#include <iomanip>
#include <algorithm>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

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
