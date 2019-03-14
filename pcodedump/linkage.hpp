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
#include <vector>
#include <memory>

namespace pcodedump {

	class CodeSegment;
	class LinkRecord;

	class LinkageInfo {
	public:
		LinkageInfo(CodeSegment & segment, const std::uint8_t * linkage);

		void write(std::wostream& os) const;

	private:
		std::vector<std::shared_ptr<LinkRecord>> linkRecords;
	};

}

#endif // !_424F2F1F_FA89_49E3_AC30_FD9BB48B47D3
