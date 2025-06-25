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

#ifndef _AC0A2AE0_F573_45FA_950F_F3D898D9994E
#define _AC0A2AE0_F573_45FA_950F_F3D898D9994E

#include <new>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <iterator>

#include <boost/endian/arithmetic.hpp>
#include <boost/align/aligned_allocator.hpp>

namespace pcodedump {

	constexpr unsigned int BLOCK_SIZE = 512;

    using buff_t = std::vector<std::uint8_t, boost::alignment::aligned_allocator<std::uint8_t, BLOCK_SIZE>>;

    template <typename T>
    T const & place(std::uint8_t const * address) {
        return *reinterpret_cast<T const *>(address);
    }

    inline std::uint8_t const * derefSelfPtr(void const * selfPtr) {
		return static_cast<std::uint8_t const *>(selfPtr) - *reinterpret_cast<boost::endian::little_int16_t const *>(selfPtr);
	}

	template <typename T>
	class Range {
	public:
		Range() : m_begin{ nullptr }, m_end{ nullptr } {}
		Range(T * begin, T * end) : m_begin{ begin }, m_end{ end } {}
		T * begin() const { return m_begin; }
		T * end() const { return m_end; }
	private:
		T * m_begin;
		T * m_end;
	};
}

#endif // !_AC0A2AE0_F573_45FA_950F_F3D898D9994E
