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
#include "pcode.hpp"
#include "native6502.hpp"
#include "types.hpp"
#include "segment.hpp"
#include <iterator>
#include <cstddef>
#include "options.hpp"

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

	CodePart::CodePart(CodeSegment & segment, std::uint8_t const * segBegin, int segLength) :
		data{segBegin, segBegin+segLength },
		procDict{ ProcedureDictionary::place(segBegin, segLength) },
		procedures { extractProcedures() }
	{
	}

	Procedure const * CodePart::findProcedure(std::uint8_t const * address) const {
		auto result = find_if(cbegin(*procedures), cend(*procedures), [address](Procedures::value_type const & proc) {return proc->contains(address); });
		if (result == cend(*procedures)) {
			return nullptr;
		} else {
			return result->get();
		}
	}

	void CodePart::writeHeader(std::wostream& os) const {
		os << L"    Procedures : " << procDict.numProcedures << endl;
	}

	bool procedureNumberOrder(shared_ptr<Procedure const> left, shared_ptr<Procedure const> right) {
		return left->getProcedureNumber() < right->getProcedureNumber();
	}

	void CodePart::disassemble(std::wostream& os) const {
		for (auto & procedure : *procedures) {
			procedure->writeHeader(begin(), os);
			if (disasmProcs) {
				procedure->disassemble(begin(), os);
				os << endl;
			}
		}
	}

	/* Get the procedure code memory ranges and construct a vector of procedure objedts.
	   The procedure pointers in the segment point to the end of each pointer.  In
	   memory this works well for the P-machine, but for disassembling the procedures
	   we need to begin at the start. Once the ranges are known, an object for each
	   procedure will be constructed with the full information.*/
	unique_ptr<CodePart::Procedures> CodePart::extractProcedures() {
		map<uint8_t const *, int> procEnds;
		for (int index = 0; index != procDict.numProcedures; ++index) {
			procEnds[procDict[index]] = index;
		}

		auto result = make_unique<Procedures>();
		auto currentStart = begin();
		for (auto[end, procNumber] : procEnds) {
			Range range(currentStart, end);
			if (*(range.end() - 2)) {
				result->push_back(make_shared<PcodeProcedure>(*this, procNumber + 1, range));
			} else {
				result->push_back(make_shared<Native6502Procedure>(*this, procNumber + 1, range));
			}
			currentStart = end;
		}

		sort(std::begin(*result), std::end(*result), procedureNumberOrder);
		return result;
	}

}
