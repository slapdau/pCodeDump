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
#include <map>
#include <string>
#include <memory>
#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	enum class SegmentKind {
		linked,
		hostseg,
		segproc,
		unitseg,
		seprtseg,
		unlinkedIntrins,
		linkedIntrins,
		dataSeg,
	};

	std::wostream& operator<<(std::wostream& os, const SegmentKind& value);

	enum class MachineType {
		undentified,
		pcode_big,
		pcode_little,
		native_pdp11,
		native_m8080,
		native_z80,
		native_ga440,
		native_m6502,
		native_m6800,
		native_tms9900,
	};

	std::wostream& operator<<(std::wostream& os, const MachineType& value);

	class SegmentDictionary;

	class CodePart;
	class InterfaceText;
	class LinkageInfo;

	class Segment {
		friend std::wostream& operator<<(std::wostream&, const Segment&);
	public:
		Segment(buff_t const & buffer, SegmentDictionary const * segmentDictionary, int index, int endBlock);

	public:

		SegmentKind getSegmentKind() const {
			return segmentKind;
		}

		int getFirstBlock() const {
			return textBlock ? textBlock : codeBlock;
		}

	private:
		
		void writeHeader(std::wostream& os) const;

		bool hasPcode() const {
			return machineType == MachineType::pcode_little && segmentKind != SegmentKind::dataSeg;
		}

		bool has6502code() const {
			return machineType == MachineType::native_m6502 && segmentKind != SegmentKind::dataSeg;
		}

		std::unique_ptr<CodePart> createCodePart();
		std::unique_ptr<InterfaceText> createInterfaceText();
		std::unique_ptr<LinkageInfo> createLinkageInfo();

	protected:
		buff_t const & buffer;
		std::wstring name;
		int textBlock;
		int codeBlock;
		int codeLength;
		int nextSegBlock;
		SegmentKind segmentKind;
		int segmentNumber;
		MachineType machineType;
		int version;
		std::unique_ptr<CodePart> codePart;
		std::unique_ptr<InterfaceText> interfaceText;
		std::unique_ptr<LinkageInfo> linkageInfo;
	};

	std::wostream& operator<<(std::wostream& os, const Segment& segment);

	using Segments = std::vector<std::shared_ptr<Segment>>;

	class PcodeFile {

		friend std::wostream& operator<<(std::wostream&, const PcodeFile&);
	public:
		PcodeFile(buff_t const & buffer);

	private:
		std::unique_ptr<Segments> extractSegments();
	private:
		buff_t const & buffer;
		SegmentDictionary const * segmentDictionary;
	public:
		std::unique_ptr<Segments> segments;
		std::uint64_t intrinsicLibraries;
		std::wstring comment;
	};

	std::wostream& operator<<(std::wostream& os, const PcodeFile& value);

}

#endif // !_773BCD58_B2D9_43BA_BC08_12754CD95096
