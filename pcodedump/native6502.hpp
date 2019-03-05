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

#ifndef _2DBE5D93_13B5_4D23_AB77_1150B342D655
#define _2DBE5D93_13B5_4D23_AB77_1150B342D655

#include "basecode.hpp"
#include "types.hpp"
#include "options.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	class Native6502Procedure : public Procedure {

		struct AttributeTable {
			boost::endian::little_uint16_t enterIc;
			boost::endian::little_uint8_t procedureNumber;
			boost::endian::little_uint8_t relocationSeg;
		};
	public:
		using base = Procedure;
		Native6502Procedure(CodePart & codePart, int procedureNumber, Range<std::uint8_t const> range);

		static void initialiseCpu(cpu_t const &);

		void writeHeader(std::uint8_t const * segBegin, std::wostream& os) const;
		void disassemble(std::uint8_t const * segBegin, std::wostream& os) const;

	private:
		using Relocations = std::vector<std::uint8_t const *>;


		std::uint8_t const * readRelocations(Relocations &table, std::uint8_t const * rawTable);
		std::uint8_t const * getEnterIc() const;

		void printIc(std::wostream& os, std::uint8_t const * current) const;

		std::uint8_t const * decode_implied(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_immedidate(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_accumulator(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_absolute(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_absoluteindirect(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_absoluteindirectindexed(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_zeropage(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_zeropageindirect(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_absoluteindexedx(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_absoluteindexedy(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_zeropageindexedx(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_zeropageindexedy(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_relative(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_indexedindirect(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_indirectindexed(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;

		using decode_function_t = std::uint8_t const * (Native6502Procedure::*)(std::wostream&, std::wstring&, std::uint8_t const *) const;
		using dispatch_t = std::tuple<std::wstring, decode_function_t>;
		static std::vector<dispatch_t> dispatch;
		static std::map<int, dispatch_t> dispatch_65c02;

		AttributeTable const * attributeTable;
		uint8_t const * procEnd;

		std::wstring formatAbsoluteAddress(std::uint8_t const * address) const;

		Relocations baseRelocations;
		Relocations segRelocations;
		Relocations procRelocations;
		Relocations interpRelocations;
	};

}

#endif // !_2DBE5D93_13B5_4D23_AB77_1150B342D655
