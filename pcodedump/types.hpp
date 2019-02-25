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

#ifndef _AC0A2AE0_F573_45FA_950F_F3D898D9994E
#define _AC0A2AE0_F573_45FA_950F_F3D898D9994E

#include <vector>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	constexpr unsigned int BLOCK_SIZE = 512;

	using buff_t = std::vector<std::uint8_t>;

	inline std::uint8_t const * derefSelfPtr(void const * selfPtr) {
		return static_cast<std::uint8_t const *>(selfPtr) - *reinterpret_cast<boost::endian::little_int16_t const *>(selfPtr);
	}

	template <typename T>
	T getNext(std::uint8_t const * & address) {
		T val = * reinterpret_cast<T const *>(address);
		address += sizeof(T);
		return val;
	}

}

#endif // !_AC0A2AE0_F573_45FA_950F_F3D898D9994E
