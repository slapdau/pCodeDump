#ifndef _3FCC8EAF_9802_4C63_9008_CA4602A96E92
#define _3FCC8EAF_9802_4C63_9008_CA4602A96E92

#include <cstdint>
#include <iostream>

namespace pcodedump {

	class SegmentDirectoryEntry;

	class TextSegment {
	public:
		TextSegment(SegmentDirectoryEntry & directoryEntry, const std::uint8_t * text);
		void write(std::wostream& os) const;
	private:
		SegmentDirectoryEntry & directoryEntry;
		const std::uint8_t * text;
	};

}

#endif // !_3FCC8EAF_9802_4C63_9008_CA4602A96E92
