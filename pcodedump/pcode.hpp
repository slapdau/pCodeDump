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

		struct AttributeTable {
			boost::endian::little_uint16_t jumpTableStart;
			boost::endian::little_uint16_t dataSize;
			boost::endian::little_uint16_t paramaterSize;
			boost::endian::little_uint16_t exitIc;
			boost::endian::little_uint16_t enterIc;
			boost::endian::little_uint8_t procedureNumber;
			boost::endian::little_uint8_t lexLevel;
		};

	public:
		using base = Procedure;
		PcodeProcedure(CodePart & codePart, int procedureNumber, Range<std::uint8_t const> range);

		std::optional<int> getLexicalLevel() const override {
			return attributeTable->lexLevel;
		}

		void writeHeader(std::wostream& os) const override;
		void disassemble(std::wostream& os, linkref_map_t & linkage) const override;

		std::uint8_t const * jtab(int index) const;
	private:
		std::uint8_t const * getEnterIc() const;
		std::uint8_t const * getExitIc() const;

		void printIc(std::wostream& os, std::uint8_t const * current)  const;

	private:
		AttributeTable const * attributeTable;
	};

}


#endif // !_00C6B89A_AA9B_40A8_8753_1DBCAD25C429
