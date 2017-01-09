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

	/* Returns the offset from some base pointer to the location pointed at by a self pointer. */
	inline int defererenceSelfPtr(std::uint8_t * base, void * selfPtr) {
		return static_cast<int>(std::distance(base, static_cast<std::uint8_t *>(selfPtr)) - *static_cast<boost::endian::little_int16_t *>(selfPtr));
	}

}

#endif // !_AC0A2AE0_F573_45FA_950F_F3D898D9994E
