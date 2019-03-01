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
#include <tuple>
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

	struct RawSegmentDirectory {
		struct {
			boost::endian::little_int16_t codeaddr;
			boost::endian::little_int16_t codeleng;
		} diskInfo[16];
		char segName[16][8];
		boost::endian::little_int16_t segKind[16];
		boost::endian::little_int16_t textAddr[16];
		boost::endian::little_int16_t segInfo[16];
		boost::endian::little_uint64_t intrinsicSegs;
		boost::endian::little_uint16_t filler[68];
		char comment[80];
	};

	class CodeSegment;
	class TextSegment;
	class LinkageSegment;

	class SegmentDirectoryEntry {
		friend std::wostream& operator<<(std::wostream&, const SegmentDirectoryEntry&);
	public:
		SegmentDirectoryEntry(buff_t const & buffer, RawSegmentDirectory const * rawDict, int index, int endBlock);

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

		std::unique_ptr<CodeSegment> createCodeSegment();
		std::unique_ptr<TextSegment> createTextSegment();
		std::unique_ptr<LinkageSegment> createLinkageSegment();

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
		std::unique_ptr<CodeSegment> codeSegment;
		std::unique_ptr<TextSegment> textSegment;
		std::unique_ptr<LinkageSegment> linkageSegment;
	};

	std::wostream& operator<<(std::wostream& os, const SegmentDirectoryEntry& value);

	using SegmentEntries = std::vector<std::shared_ptr<SegmentDirectoryEntry>>;

	class SegmentDirectory {

		friend std::wostream& operator<<(std::wostream&, const SegmentDirectory&);
	public:
		SegmentDirectory(buff_t const & buffer);

	private:
		std::unique_ptr<SegmentEntries> extractDirectoryEntries();
	private:
		buff_t const & buffer;
		RawSegmentDirectory const * rawDirectory;
	public:
		std::unique_ptr<SegmentEntries> entries;
		std::uint64_t intrinsicLibraries;
		std::wstring comment;
	};

	std::wostream& operator<<(std::wostream& os, const SegmentDirectory& value);

}

#endif // !_773BCD58_B2D9_43BA_BC08_12754CD95096
