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

#include "basecode.hpp"
#include "types.hpp"
#include "directory.hpp"
#include <iterator>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	CodeSegment::CodeSegment(SegmentDirectoryEntry & directoryEntry, std::uint8_t * segBegin, int segLength) :
		segBegin{ segBegin }, segLength{ segLength },
		header{ reinterpret_cast<RawProcedureDirectoryHead *>(segBegin + segLength - sizeof(RawProcedureDirectoryHead)) }
	{}

	CodeSegment::~CodeSegment() {}

	/* Gets a vector of the procedure ranges in this code segment.  The tuples are
	    - procedure number
	    - Pointer to start
		- Length
	   The procedure pointers in the segment point to the end of each pointer.  In
	   memory this works well for the P-machine, but for disassembling the procedures
	   we need to begin at the start. Once the ranges are known, an object for each
	   procedure will be constructed with the full information. */
	vector<CodeSegment::ProcRange> CodeSegment::getProcRanges() {
		vector<tuple<int, int>> procEnd;
		auto procZeroPtr = reinterpret_cast<little_int16_t *>(segBegin + segLength - sizeof(RawProcedureDirectoryHead)) - 1;
		for (int index = 0; index != header->numProcedures; ++index) {
			int end = defererenceSelfPtr(segBegin, procZeroPtr - index) + sizeof(little_int16_t);
			procEnd.emplace_back(make_tuple(index, end));
		}
		// Sort by address ascending.
		sort(std::begin(procEnd), std::end(procEnd), [](const auto & left, const auto & right) { return get<1>(left) < get<1>(right); });
		int currentStart = 0;
		vector<ProcRange> procRange;
		transform(std::begin(procEnd), std::end(procEnd), back_inserter(procRange), [&currentStart, this](const auto & value) {
			auto [procNumber, end] = value;
			auto result = make_tuple(procNumber, segBegin + currentStart, end - currentStart);
			currentStart = end;
			return result;
		});
		// Sort by procedure number ascending.  Restore order from disk file.
		sort(std::begin(procRange), end(procRange), [](const auto & left, const auto & right) { return get<0>(left) < get<0>(right); });
		return procRange;
	}

	void CodeSegment::writeHeader(std::wostream& os) const {
		os << L"    Procedures : " << getNumProcedures() << endl;
	}
}
