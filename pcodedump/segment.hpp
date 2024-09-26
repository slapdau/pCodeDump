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

#ifndef _4F5901F5_E50C_44CC_BCC3_861305540578
#define _4F5901F5_E50C_44CC_BCC3_861305540578

#include "types.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
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

	// http://www.unige.ch/medecine/nouspikel/ti99/psystem.htm#Segment%20info
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

	class SegmentDictionary {
		friend class SegmentDictionaryEntry;
	public:
		static constexpr int NUM_SEGMENTS = 16;
		SegmentDictionaryEntry const operator[](int index) const;

		uint64_t intrinsicSegments() const;
		std::wstring fileComment() const;

	private:
		struct {
			boost::endian::little_int16_t codeaddr;
			boost::endian::little_int16_t codeleng;
		} diskInfo[NUM_SEGMENTS];
		char segName[NUM_SEGMENTS][8];
		boost::endian::little_int16_t segKind[NUM_SEGMENTS];
		boost::endian::little_int16_t textAddr[NUM_SEGMENTS];
		boost::endian::little_int16_t segInfo[NUM_SEGMENTS];
		boost::endian::little_uint64_t intrinsicSegs;
		boost::endian::little_uint16_t filler[68];
		char comment[80];
	};

	class SegmentDictionaryEntry {
		friend class SegmentDictionary;

	private:
		SegmentDictionaryEntry(SegmentDictionary const * segmentDictionary, int index);

	public:
		int getIndex() const { return index; }
		int codeAddress() const;
		int codeLength() const;
		std::wstring name() const;
		int textAddress() const;
		SegmentKind segmentKind() const;
		int segmentNumber() const;
		MachineType machineType() const;
		int version() const;

		int startAddress() const;
		int linkageAddress() const;
	private:
		SegmentDictionary const * segmentDictionary;
		int index;
	};

	class Segment {
	public:
		Segment(SegmentDictionaryEntry const dictionaryEntry);
		virtual ~Segment() = 0;

	public:
		int getDictionaryIndex() const {
			return dictionaryEntry.getIndex();
		}

		int getSegmentNumber() const {
			return dictionaryEntry.segmentNumber();
		}

		SegmentKind getSegmentKind() const {
			return dictionaryEntry.segmentKind();
		}

		virtual int getFirstBlock() const = 0;
		virtual std::wostream& writeOut(std::wostream&) const;

	protected:
		SegmentDictionaryEntry const dictionaryEntry;
	};

	std::wostream& operator<<(std::wostream&, const Segment&);


	class DataSegment : public Segment {
	public:
		DataSegment(SegmentDictionaryEntry const dictionaryEntry);

		int getFirstBlock() const override {
			return 0;
		}
	};
	
	class CodePart;
	class InterfaceText;
	class LinkageInfo;

	class CodeSegment : public Segment {
	public:
		CodeSegment(buff_t const & buffer, SegmentDictionaryEntry const dictionaryEntry, int endBlock);

		int getFirstBlock() const override {
			return dictionaryEntry.startAddress();
		}

		std::wostream& writeOut(std::wostream&) const override;
		bool detailEnabled() const;

	private:
		void writeHeader(std::wostream& os) const;
		std::unique_ptr<CodePart> createCodePart();
		std::unique_ptr<InterfaceText> createInterfaceText();
		std::unique_ptr<LinkageInfo> createLinkageInfo();

	private:
		buff_t const & buffer;
		int endBlock;
		std::unique_ptr<CodePart> codePart;
		std::unique_ptr<InterfaceText> interfaceText;
		std::unique_ptr<LinkageInfo> linkageInfo;

	public:
		static bool showText;
		static bool listProcs;
		static bool showLinkage;
		static std::vector<int> segments;
	};

}

#endif // !_4F5901F5_E50C_44CC_BCC3_861305540578
