/*
   Copyright 2017-2023 Craig McGeachie

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

#ifndef _0058CB76_8CFA_4C70_8961_2F643D0EF3FB
#define _0058CB76_8CFA_4C70_8961_2F643D0EF3FB

#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <boost/endian/arithmetic.hpp>
#include "types.hpp"

namespace pcodedump {

	class CodePart;
	class LinkageInfo;
	class LinkRecord;

	using linkref_map_t = std::map<std::uint8_t const *, std::shared_ptr<LinkRecord const>>;

	class Procedure {
	public:
		Procedure(CodePart & codePart, int procedureNumber, Range<std::uint8_t const> data) :
			codePart{ codePart }, procedureNumber{ procedureNumber }, data{ data }
		{}

		virtual void writeHeader(std::wostream& os) const = 0;
		virtual void disassemble(std::wostream& os, linkref_map_t & linkage) const = 0;

		virtual ~Procedure() = default;
		
		int getProcedureNumber() const {
			return procedureNumber;
		}

		virtual std::optional<int> getLexicalLevel() const = 0;

		std::uint8_t const * getProcBegin() const {
			return data.begin();
		}

		bool contains(std::uint8_t const * address) const {
			return data.begin() <= address && address < data.end();
		}

	protected:
		CodePart const & codePart;
		int const procedureNumber;
		Range<std::uint8_t const> data;
	};

	class ScopeNode;
	using ScopeNodes = std::vector<std::shared_ptr<ScopeNode>>;

	class ScopeNode {
	public:
		explicit ScopeNode(std::shared_ptr<Procedure const> procedure);

		void add(std::shared_ptr<ScopeNode> child);

		int getLexicalLevel() const {
			return procedure->getLexicalLevel().value();
		}

		void writeOut(std::wostream& os, std::wstring prefix) const;

	private:
		std::shared_ptr<Procedure const> procedure;
		std::unique_ptr<ScopeNodes> children;
	};


	class CodeSegment;
	class ProcedureDictionary;

	class CodePart final {
	public:
		CodePart() = delete;
		CodePart(const CodePart &) = delete;
		CodePart(const CodePart &&) = delete;

		CodePart(CodeSegment & segment, std::uint8_t const * segBegin, int segLength);

		uint8_t const * begin() const {
			return data.begin();
		}

		void writeHeader(std::wostream& os) const;
		void disassemble(std::wostream& os, LinkageInfo * linkageInfo) const;
		Procedure const * findProcedure(std::uint8_t const * address) const;


	private:
		using Procedures = std::vector<std::shared_ptr<Procedure const>>;
		std::unique_ptr<Procedures> extractProcedures();
		std::shared_ptr<ScopeNode> extractTree();

	private:
		Range<std::uint8_t const> data;
		ProcedureDictionary const & procDict;
		std::unique_ptr<Procedures const> procedures;
		std::shared_ptr<ScopeNode> treeRoot;

	public:
		static bool treeProcs;
		static bool disasmProcs;
	};

}

#endif // !_0058CB76_8CFA_4C70_8961_2F643D0EF3FB
