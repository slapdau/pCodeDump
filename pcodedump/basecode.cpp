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
#include <cstddef>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	class ProcedureDictionary final {
	public:
		static ProcedureDictionary const & place(std::uint8_t const * segStart, int segLength) {
			return *reinterpret_cast<ProcedureDictionary const *>(segStart + segLength - sizeof(ProcedureDictionary));
		}

		std::uint8_t const * operator[](int index) const;


	private:
		// Restrict stack allocation.
		~ProcedureDictionary() = default;

		// Restrict normal heap allocation
		void * operator new(std::size_t) noexcept { return nullptr; }
		void operator delete(void * ptr, std::size_t) noexcept {}
		void * operator new[](std::size_t) noexcept { return nullptr; }
			void operator delete[](void * ptr, std::size_t) noexcept {}


	public:
		boost::endian::little_uint8_t const segmentNumber;
		boost::endian::little_uint8_t const numProcedures;
	};

	std::uint8_t const * ProcedureDictionary::operator[](int index) const
	{
		return derefSelfPtr(reinterpret_cast<std::uint8_t const *>(this) - 2 - 2 * index) + sizeof(little_int16_t);
	}

	CodeSegment::CodeSegment(SegmentDirectoryEntry & directoryEntry, std::uint8_t const * segBegin, int segLength) :
		data{segBegin, segBegin+segLength }, procDict{ ProcedureDictionary::place(segBegin, segLength) }
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
	map<int, tuple<uint8_t const *, intptr_t>> CodeSegment::getProcRanges() {
		map<uint8_t const *, int> procEnds;
		for (int index = 0; index != procDict.numProcedures; ++index) {
			procEnds[procDict[index]] = index;
		}

		auto currentStart = begin();
		map<int, tuple<uint8_t const *, intptr_t>> procRanges;
		for (auto procEnd : procEnds) {
			auto[end, procNumber] = procEnd;
			procRanges[procNumber] = make_tuple(currentStart, end - currentStart);
			currentStart = end;
		}

		return move(procRanges);
	}

	Procedure * CodeSegment::findProcedure(std::uint8_t const * address) const {
		auto result = find_if(cbegin(*entries), cend(*entries), [address](Procedures::value_type const & proc) {return proc->contains(address); });
		if (result == cend(*entries)) {
			return nullptr;
		} else {
			return result->get();
		}
	}


	void CodeSegment::writeHeader(std::wostream& os) const {
		os << L"    Procedures : " << procDict.numProcedures << endl;
	}

}
