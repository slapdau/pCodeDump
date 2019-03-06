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

#ifndef _773BCD58_B2D9_43BA_BC08_12754CD95096
#define _773BCD58_B2D9_43BA_BC08_12754CD95096

#include "types.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <vector>

namespace pcodedump {

	class CodeSegment;
	class SegmentDictionary;

	using Segments = std::vector<std::shared_ptr<CodeSegment>>;

	class PcodeFile {
		friend std::wostream& operator<<(std::wostream&, const PcodeFile&);

	public:
		PcodeFile(buff_t const & buffer);

	private:
		std::unique_ptr<Segments> extractSegments();
	
	private:
		buff_t const & buffer;
		SegmentDictionary const & segmentDictionary;
		std::unique_ptr<Segments> segments;
	};

	std::wostream& operator<<(std::wostream& os, const PcodeFile& value);

}

#endif // !_773BCD58_B2D9_43BA_BC08_12754CD95096
