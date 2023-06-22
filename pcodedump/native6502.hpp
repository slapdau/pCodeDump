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

		std::optional<int> getLexicalLevel() const override {
			return std::nullopt;
		}

		void writeHeader(std::wostream& os) const override;
		void disassemble(std::wostream& os, linkref_map_t & linkage) const override;

	private:
		using Relocations = std::vector<std::uint8_t const *>;


		std::uint8_t const * readRelocations(Relocations &table, std::uint8_t const * rawTable);
		std::uint8_t const * getEnterIc() const;

		void printIc(std::wostream& os, std::uint8_t const * current) const;

		AttributeTable const * attributeTable;
		uint8_t const * procEnd;

	public:

	private:
		Relocations baseRelocations;
		Relocations segRelocations;
		Relocations procRelocations;
		Relocations interpRelocations;
		
		class Disassembler;
	};

}

#endif // !_2DBE5D93_13B5_4D23_AB77_1150B342D655
