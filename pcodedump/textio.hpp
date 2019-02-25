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

#ifndef _73FA3A84_4431_426F_A727_612E03F69820
#define _73FA3A84_4431_426F_A727_612E03F69820

#include <cstdint>
#include <string>
#include <ios>
#include "types.hpp"

namespace pcodedump {

	template<typename T>
	class FmtSentry {
		using ios_t = std::basic_ios<T>;
		using fmtflags = typename ios_t::fmtflags;


	public:
		FmtSentry(ios_t &stream) : flags{ stream.flags() }, stream{ stream } {}
		~FmtSentry() {
			stream.flags(flags);
		}

	private:
		fmtflags flags;
		ios_t &stream;
	};

	void line_hexdump(std::uint8_t const * start, std::uint8_t const * finish);

	void line_chardump(std::uint8_t const * start, std::uint8_t const * finish);

	void hexdump(std::wstring leader, std::uint8_t const * start, std::uint8_t const * finish);

	void line_hexdump(buff_t::iterator start, buff_t::iterator finish);

	void line_chardump(buff_t::iterator start, buff_t::iterator finish);

	void hexdump(std::wstring leader, buff_t::iterator start, buff_t::iterator finish);

	void hexdump(buff_t &buffer);

}


#endif // !_73FA3A84_4431_426F_A727_612E03F69820
