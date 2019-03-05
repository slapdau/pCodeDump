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

		void writeHeader(std::uint8_t const * segBegin, std::wostream& os) const;
		void disassemble(std::uint8_t const * segBegin, std::wostream& os) const;

	private:
		std::uint8_t const * getEnterIc() const;
		std::uint8_t const * getExitIc() const;

		void printIc(std::wostream& os, std::uint8_t const * current)  const;

		std::uint8_t const * jtab(int index) const;

		std::uint8_t const * decode_implied(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_unsignedByte(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_big(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_intermediate(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_extended(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_word(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_wordBlock(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_stringConstant(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_packedConstant(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_jump(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_return(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_doubleByte(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_case(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_callStandardProc(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;
		std::uint8_t const * decode_compare(std::wostream& os, std::wstring &opCode, std::uint8_t const * current) const;

		using decode_function_t = std::uint8_t const * (PcodeProcedure::*)(std::wostream&, std::wstring&, std::uint8_t const *) const;

		using dispatch_t = std::tuple<std::wstring, decode_function_t>;

		static std::vector<dispatch_t> dispatch;

	private:
		AttributeTable const * attributeTable;
	};

}


#endif // !_00C6B89A_AA9B_40A8_8753_1DBCAD25C429
