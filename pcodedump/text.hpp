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

#ifndef _3FCC8EAF_9802_4C63_9008_CA4602A96E92
#define _3FCC8EAF_9802_4C63_9008_CA4602A96E92

#include <cstdint>
#include <iostream>

namespace pcodedump {

	class Segment;

	class InterfaceText {
	public:
		InterfaceText(Segment & segment, const std::uint8_t * text);
		void write(std::wostream& os) const;
	private:
		Segment const & segment;
		const std::uint8_t * text;
	};

}

#endif // !_3FCC8EAF_9802_4C63_9008_CA4602A96E92
