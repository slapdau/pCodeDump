#ifndef _0058CB76_8CFA_4C70_8961_2F643D0EF3FB
#define _0058CB76_8CFA_4C70_8961_2F643D0EF3FB

#include <cstdint>
#include <tuple>
#include <vector>
#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	class CodeSegment;

	class Procedure {
	public:
		Procedure(CodeSegment & segment, int procedureNumber, std::uint8_t * procBegin, int procLength) :
			segment{ segment }, procedureNumber{ procedureNumber }, procBegin{ procBegin }, procLength{ procLength }
		{}

		virtual void writeHeader(std::uint8_t* segBegin, std::wostream& os) const = 0;
		virtual void disassemble(std::uint8_t* segBegin, std::wostream& os) const = 0;

		virtual ~Procedure() {}

		std::uint8_t * getProcBegin() { return procBegin; }

	protected:
		CodeSegment & segment;
		int procedureNumber;
		std::uint8_t * procBegin;
		int procLength;
	};

	class SegmentDirectoryEntry;

	class CodeSegment {
		struct RawProcedureDirectoryHead {
			boost::endian::little_uint8_t segmentNumber;
			boost::endian::little_uint8_t numProcedures;
		};

	public:
		CodeSegment() = delete;
		CodeSegment(const CodeSegment &) = delete;
		CodeSegment(const CodeSegment &&) = delete;

		CodeSegment(SegmentDirectoryEntry & directoryEntry, std::uint8_t * segBegin, int segLength);

		virtual ~CodeSegment();

		int getSegmentNumber() const {
			return header->segmentNumber;
		}

		int getNumProcedures() const {
			return header->numProcedures;
		}

		uint8_t * begin() const {
			return segBegin;
		}

		virtual void writeHeader(std::wostream& os) const;
		virtual void disassemble(std::wostream& os) const = 0;

	protected:
		using ProcRange = std::tuple<int, std::uint8_t *, int>; // Procedure number, start, length
		std::vector<ProcRange> getProcRanges();

	protected:
		std::uint8_t * segBegin;
		int segLength;

	private:
		RawProcedureDirectoryHead * header;
	};

}

#endif // !_0058CB76_8CFA_4C70_8961_2F643D0EF3FB
