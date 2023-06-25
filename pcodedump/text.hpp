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

#ifndef _3FCC8EAF_9802_4C63_9008_CA4602A96E92
#define _3FCC8EAF_9802_4C63_9008_CA4602A96E92

#include <cstdint>
#include <iostream>
#include <tuple>
#include <string>

namespace pcodedump {

	class CodeSegment;

	class InterfaceText {
	public:
		InterfaceText(CodeSegment & segment, const std::uint8_t * begin, const std::uint8_t * end);
		void write(std::wostream& os) const;

	private:
		std::tuple<std::wstring, const uint8_t*> readline(const uint8_t* input) const;

		CodeSegment const & segment;
		const std::uint8_t * begin;
		const std::uint8_t * end;
	};

}

#endif // !_3FCC8EAF_9802_4C63_9008_CA4602A96E92
