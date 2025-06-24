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

#ifndef _00C6B89A_AA9B_40A8_8753_1DBCAD25C429
#define _00C6B89A_AA9B_40A8_8753_1DBCAD25C429

#include "basecode.hpp"
#include "types.hpp"
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <memory>

#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	class PcodeProcedure : public Procedure {

	public:
		using base = Procedure;
		PcodeProcedure(CodePart & codePart, int procedureNumber, Range<std::uint8_t const> range);

		std::optional<int> getLexicalLevel() const override;

		void writeHeader(std::wostream& os) const override;
		void disassemble(std::wostream& os, linkref_map_t & linkage) const override;

		std::uint8_t const * jtab(int index) const;
	private:
		std::uint8_t const * getEnterIc() const;
		std::uint8_t const * getExitIc() const;

		void printIc(std::wostream& os, std::uint8_t const * current)  const;

	private:
		class AttributeTable;
		AttributeTable const & attributeTable;

		class Disassembler;
	};

	float convertToReal(std::uint8_t const * buff);

}

#endif // !_00C6B89A_AA9B_40A8_8753_1DBCAD25C429
