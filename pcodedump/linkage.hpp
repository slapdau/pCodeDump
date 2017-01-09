#ifndef _424F2F1F_FA89_49E3_AC30_FD9BB48B47D3
#define _424F2F1F_FA89_49E3_AC30_FD9BB48B47D3

#include <cstdint>
#include <iostream>
#include <map>

namespace pcodedump {

	class SegmentDirectoryEntry;

	enum class LinkageType { eofMark, unitRef, globRef, publRef, privRef, constRef, globDef, publDef, constDef, extProc, extFunc, sepProc, sepFunc, seppRef, sepfRef };

	class LinkageSegment {
	public:
		LinkageSegment(SegmentDirectoryEntry & directoryEntry, const std::uint8_t * linkage);

		void write(std::wostream& os) const;

	private:
		using decode_function_t = std::uint8_t const * (LinkageSegment::*)(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_reference(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_privateReference(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_globalDefinition(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_publicDefinition(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_constantDefinition(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_externalRoutine(std::wostream&, std::uint8_t const *) const;
		std::uint8_t const * decode_endOfFile(std::wostream&, std::uint8_t const *) const;

		std::uint8_t const * decode_referenceArray(std::wostream&, std::uint8_t const *, int) const;

		static std::map<LinkageType, std::tuple<std::wstring, decode_function_t>> linkageNames;

	private:
		SegmentDirectoryEntry & directoryEntry;
		const std::uint8_t * linkage;
	};

}

#endif // !_424F2F1F_FA89_49E3_AC30_FD9BB48B47D3
