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

#ifndef _0058CB76_8CFA_4C70_8961_2F643D0EF3FB
#define _0058CB76_8CFA_4C70_8961_2F643D0EF3FB

#include <cstdint>
#include <tuple>
#include <vector>
#include <map>
#include <memory>
#include <boost/endian/arithmetic.hpp>
#include "types.hpp"

namespace pcodedump {

	class CodeSegment;

	class Procedure {
	public:
		Procedure(CodeSegment & segment, int procedureNumber, std::uint8_t const * procBegin, int procLength) :
			segment{ segment }, procedureNumber{ procedureNumber }, procBegin{ procBegin }, procLength{ procLength }
		{}

		virtual void writeHeader(std::uint8_t const * segBegin, std::wostream& os) const = 0;
		virtual void disassemble(std::uint8_t const * segBegin, std::wostream& os) const = 0;

		virtual ~Procedure() {}
		
		int getProcedureNumber() const {
			return procedureNumber;
		}

		std::uint8_t const * getProcBegin() const {
			return procBegin;
		}

		bool contains(std::uint8_t const * address) const {
			return procBegin <= address && address < procBegin + procLength;
		}

	protected:
		CodeSegment const & segment;
		int const procedureNumber;
		std::uint8_t const * const procBegin;
		int const procLength;
	};

	using Procedures = std::vector<std::shared_ptr<Procedure>>;

	class SegmentDirectoryEntry;
	class ProcedureDictionary;

	class CodeSegment {
	public:
		CodeSegment() = delete;
		CodeSegment(const CodeSegment &) = delete;
		CodeSegment(const CodeSegment &&) = delete;

		CodeSegment(SegmentDirectoryEntry & directoryEntry, std::uint8_t const * segBegin, int segLength);

		virtual ~CodeSegment() = 0;

		uint8_t const * begin() const {
			return data.begin();
		}

		virtual void writeHeader(std::wostream& os) const;
		virtual void disassemble(std::wostream& os) const = 0;
		Procedure * findProcedure(std::uint8_t const * address) const;


	protected:
		using ProcRange = std::tuple<int, std::uint8_t const *, int>; // Procedure number, start, length
		std::map<int, std::tuple<std::uint8_t const *, std::intptr_t>> getProcRanges();
		virtual std::unique_ptr<Procedures> initProcedures() = 0;


	protected:
		Range<std::uint8_t const> data;

	private:
		ProcedureDictionary const & procDict;

	protected:
		std::unique_ptr<Procedures> entries;
	};

}

#endif // !_0058CB76_8CFA_4C70_8961_2F643D0EF3FB
