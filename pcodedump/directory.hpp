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

	class SegmentDictionaryEntry {
		friend class SegmentDictionary;
	
	private:
		SegmentDictionaryEntry(SegmentDictionary const * segmentDictionary, int index);

	public:
		int codeAddress() const;
		int codeLength() const;
		std::wstring name() const;
		int textAddress() const;
		SegmentKind segmentKind() const;
		int segmentNumber() const;
		MachineType machineType() const;
		int version() const;

		int startAddress() const;
	private:
		SegmentDictionary const * segmentDictionary;
		int index;
	};

	class CodePart;
	class InterfaceText;
	class LinkageInfo;

	class Segment {
		friend std::wostream& operator<<(std::wostream&, const Segment&);
	public:
		Segment(buff_t const & buffer, SegmentDictionaryEntry const dictionaryEntry, int endBlock);

	public:

		SegmentKind getSegmentKind() const {
			return dictionaryEntry.segmentKind();
		}

		int getFirstBlock() const {
			return dictionaryEntry.startAddress();
		}

	private:
		
		void writeHeader(std::wostream& os) const;
		std::unique_ptr<CodePart> createCodePart();
		std::unique_ptr<InterfaceText> createInterfaceText();
		std::unique_ptr<LinkageInfo> createLinkageInfo();

	protected:
		buff_t const & buffer;
		SegmentDictionaryEntry const dictionaryEntry;
		int nextSegBlock;
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
		SegmentDictionary const & segmentDictionary;
		std::unique_ptr<Segments> segments;
	};

	std::wostream& operator<<(std::wostream& os, const PcodeFile& value);

}

#endif // !_773BCD58_B2D9_43BA_BC08_12754CD95096
