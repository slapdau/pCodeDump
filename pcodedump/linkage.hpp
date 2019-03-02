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

#ifndef _424F2F1F_FA89_49E3_AC30_FD9BB48B47D3
#define _424F2F1F_FA89_49E3_AC30_FD9BB48B47D3

#include <cstdint>
#include <iostream>
#include <map>

namespace pcodedump {

	class Segment;

	enum class LinkageType { eofMark, unitRef, globRef, publRef, privRef, constRef, globDef, publDef, constDef, extProc, extFunc, sepProc, sepFunc, seppRef, sepfRef };

	class LinkageInfo {
	public:
		LinkageInfo(Segment & segment, const std::uint8_t * linkage);

		void write(std::wostream& os) const;

	private:
		using decode_function_t = std::uint8_t const * (LinkageInfo::*)(std::wostream&, std::uint8_t const *) const;
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
		Segment & segment;
		const std::uint8_t * linkage;
	};

}

#endif // !_424F2F1F_FA89_49E3_AC30_FD9BB48B47D3
