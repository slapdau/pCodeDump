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

namespace pcodedump {

	constexpr unsigned int BLOCK_SIZE = 512;

    template<typename T, std::align_val_t ALIGNMENT = static_cast<std::align_val_t>(sizeof(std::max_align_t))>
    class AlignedAllocator
    {
        static_assert(static_cast<std::size_t>(ALIGNMENT) >= alignof(T), "Overalignment size too small for type T");

    public:
        using value_type = T;
        static std::align_val_t constexpr ALIGNMENT{ ALIGNMENT };

        using size_type = std::size_t;

        template<class U>
        struct rebind
        {
            using other = AlignedAllocator<U, ALIGNMENT>;
        };

        constexpr AlignedAllocator() noexcept = default;

        constexpr AlignedAllocator(const AlignedAllocator&) noexcept = default;

        template<typename U>
        constexpr AlignedAllocator(AlignedAllocator<U, ALIGNMENT> const&) noexcept {}

        static constexpr size_type max_size() {
            return std::numeric_limits<std::size_t>::max() / sizeof(T);
        }

        [[nodiscard]] T* allocate(std::size_t n) {
            if (n > max_size()) {
                throw std::bad_array_new_length();
            }
            return reinterpret_cast<T*>(::operator new[](n * sizeof(T), ALIGNMENT));
        }

        void
            deallocate(T* p, [[maybe_unused]] std::size_t n)
        {
            ::operator delete[](p, ALIGNMENT);
        }
    };

    using buff_t = std::vector<std::uint8_t, AlignedAllocator<std::uint8_t, static_cast<std::align_val_t>(BLOCK_SIZE)>>;

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
