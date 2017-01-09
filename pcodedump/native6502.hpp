#ifndef _2DBE5D93_13B5_4D23_AB77_1150B342D655
#define _2DBE5D93_13B5_4D23_AB77_1150B342D655

#include "basecode.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	class Native6502Segment;

	class Native6502Procedure : public Procedure {

		struct RawNative6502AttributeTable {
			boost::endian::little_uint16_t enterIc;
			boost::endian::little_uint8_t procedureNumber;
			boost::endian::little_uint8_t relocationSeg;
		};
	public:
		using base = Procedure;
		Native6502Procedure(CodeSegment & segment, int procedureNumber, std::uint8_t * procBegin, int procLength);

		static void initialiseCpu();

		void writeHeader(std::uint8_t* segBegin, std::wostream& os) const;
		void disassemble(std::uint8_t* segBegin, std::wostream& os) const;

	private:
		uint8_t * readRelocations(std::vector<int> &table, std::uint8_t * rawTable);
		int getEnterIc() const;

		void printIc(std::wostream& os, std::uint8_t * current) const;

		std::uint8_t * decode_implied(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_immedidate(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_accumulator(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_absolute(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_absoluteindirect(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_absoluteindirectindexed(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_zeropage(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_zeropageindirect(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_absoluteindexedx(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_absoluteindexedy(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_zeropageindexedx(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_zeropageindexedy(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_relative(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_indexedindirect(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;
		std::uint8_t * decode_indirectindexed(std::wostream& os, std::wstring &opCode, std::uint8_t * current) const;

		using decode_function_t = std::uint8_t* (Native6502Procedure::*)(std::wostream&, std::wstring&, std::uint8_t *) const;
		using dispatch_t = std::tuple<std::wstring, decode_function_t>;
		static std::vector<dispatch_t> dispatch;
		static std::map<int, dispatch_t> dispatch_65c02;

		RawNative6502AttributeTable * rawAttributeTable;
		uint8_t * procEnd;

		void relocateProcAddress(int address);
		std::wstring formatAbsoluteAddress(std::uint8_t * address) const;

		std::vector<int> baseRelocations;
		std::vector<int> segRelocations;
		std::vector<int> procRelocations;
		std::vector<int> interpRelocations;
	};

	using Procedures = std::vector<std::shared_ptr<Procedure>>;

	class Native6502Segment : public CodeSegment {

	public:
		using base = CodeSegment;
		Native6502Segment(SegmentDirectoryEntry & directoryEntry, std::uint8_t * segBegin, int segLength);

		void disassemble(std::wostream& os) const override;

	private:
		std::unique_ptr<Procedures> initProcedures();
		std::unique_ptr<Procedures> entries;
	};

}

#endif // !_2DBE5D93_13B5_4D23_AB77_1150B342D655
