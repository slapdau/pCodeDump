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

#include "basecode.hpp"
#include "pcode.hpp"
#include "native6502.hpp"
#include "types.hpp"
#include "segment.hpp"
#include "linkage.hpp"
#include <iterator>
#include <cstddef>

#include <stack>
#include <optional>
#include <string>
#include <algorithm>

using namespace std;
using namespace boost::endian;

namespace pcodedump {

	class ProcedureDictionary final {
	public:
		static ProcedureDictionary const & place(std::uint8_t const * segStart, int segLength) {
			return *reinterpret_cast<ProcedureDictionary const *>(segStart + segLength - sizeof(ProcedureDictionary));
		}

		std::uint8_t const * operator[](int index) const;


	private:
		// Restrict stack allocation.
		~ProcedureDictionary() = default;

		// Restrict normal heap allocation
		void * operator new(std::size_t) noexcept { return nullptr; }
		void operator delete(void * ptr, std::size_t) noexcept {}
		void * operator new[](std::size_t) noexcept { return nullptr; }
		void operator delete[](void * ptr, std::size_t) noexcept {}


	public:
		boost::endian::little_uint8_t const segmentNumber;
		boost::endian::little_uint8_t const numProcedures;
	};

	std::uint8_t const * ProcedureDictionary::operator[](int index) const
	{
		return derefSelfPtr(reinterpret_cast<std::uint8_t const *>(this) - 2 - 2 * index) + sizeof(little_int16_t);
	}

	CodePart::CodePart(CodeSegment & segment, std::uint8_t const * segBegin, int segLength) :
		segment{ segment },
		data{ segBegin, segBegin + segLength },
		procDict{ ProcedureDictionary::place(segBegin, segLength) },
		procedures{ extractProcedures() }, treeRoot{ extractTree() }
	{
	}

	Procedure const * CodePart::findProcedure(std::uint8_t const * address) const {
		auto result = find_if(cbegin(*procedures), cend(*procedures), [address](Procedures::value_type const & proc) {return proc->contains(address); });
		if (result == cend(*procedures)) {
			return nullptr;
		} else {
			return result->get();
		}
	}

	void CodePart::writeHeader(std::wostream& os) const {
		os << L"    Procedures : " << procDict.numProcedures << endl;
	}

	bool procedureNumberOrder(shared_ptr<Procedure const> left, shared_ptr<Procedure const> right) {
		return left->getProcedureNumber() < right->getProcedureNumber();
	}

	bool addressOrder(shared_ptr<Procedure const> left, shared_ptr<Procedure const> right) {
		return left->getProcBegin() < right->getProcBegin();
	}

	auto getCodeReferences(uint8_t const * codeBase, LinkageInfo * linkageInfo) {
		linkref_map_t result;
		if (linkageInfo != nullptr) {
			for (auto linkRecord : linkageInfo->getLinkRecords()) {
				auto linkReference = dynamic_cast<LinkReference const *>(linkRecord.get());
				if (linkReference != nullptr) {
					for (intptr_t reference : linkReference->getReferences()) {
						result[codeBase + reference] = linkRecord;
					}
				}
			}
		}
		return result;
	}

	bool CodePart::disasmProcs = false;
	bool CodePart::treeProcs = false;

	void CodePart::disassemble(std::wostream& os, LinkageInfo * linkageInfo) const {
		if (treeProcs && treeRoot) {
			treeRoot->writeOut(os, L"");
			os << endl;
		}
		if (!(treeProcs && treeRoot) || disasmProcs) {
			for (auto & procedure : *procedures) {
				procedure->writeHeader(os);
				if (disasmProcs) {
					auto references = getCodeReferences(this->begin(), linkageInfo);
					procedure->disassemble(os, references);
					os << endl;
				}
			}
		}
	}

	ScopeNode::ScopeNode(std::shared_ptr<Procedure const> procedure) : procedure{ procedure }, children{ std::make_unique<ScopeNodes>() }
	{
	}

	void ScopeNode::add(shared_ptr<ScopeNode> child) {
		children->insert(begin(*children), child);
	}

	void ScopeNode::writeOut(std::wostream& os, std::wstring prefix) const {
		procedure->writeHeader(os);
		if (!children->empty()) {
			for (auto child = cbegin(*children); child != cend(*children) - 1; ++child) {
				os << prefix << L" |--";
				(*child)->writeOut(os, prefix + wstring{ L" |  " });
			}
			auto child = cend(*children) - 1;
			os << prefix << L" \\--";
			(*child)->writeOut(os, prefix + wstring{ L"    " });
		}

	}

	/* Get the procedure code memory ranges and construct a vector of procedure objedts.
	   The procedure pointers in the segment point to the end of each pointer.  In
	   memory this works well for the P-machine, but for disassembling the procedures
	   we need to begin at the start. Once the ranges are known, an object for each
	   procedure will be constructed with the full information.*/
	unique_ptr<CodePart::Procedures> CodePart::extractProcedures() {
		if (!segment.detailEnabled()) {
			return unique_ptr<CodePart::Procedures>();
		} else {
			map<uint8_t const *, int> procEnds;
			for (int index = 0; index != procDict.numProcedures; ++index) {
				procEnds[procDict[index]] = index;
			}

			auto result = make_unique<Procedures>();
			auto currentStart = begin();
			for (auto[end, procNumber] : procEnds) {
				Range range(currentStart, end);
				if (*(range.end() - 2)) {
					result->push_back(make_shared<PcodeProcedure>(*this, procNumber + 1, range));
				} else {
					result->push_back(make_shared<Native6502Procedure>(*this, procNumber + 1, range));
				}
				currentStart = end;
			}

			return result;
		}
	}

	/* Get the procedure code memory ranges and construct a vector of procedure objedts.
	   The procedure pointers in the segment point to the end of each pointer.  In
	   memory this works well for the P-machine, but for disassembling the procedures
	   we need to begin at the start. Once the ranges are known, an object for each
	   procedure will be constructed with the full information.*/
	shared_ptr<ScopeNode> CodePart::extractTree() {
		if (!segment.detailEnabled()) {
			return shared_ptr<ScopeNode>();
		} else {
			Procedures  procedures{ *(this->procedures) };
			sort(::std::begin(procedures), ::std::end(procedures), addressOrder);
			stack<shared_ptr<ScopeNode>> nativeStack;
			stack<shared_ptr<ScopeNode>> pcodeStack;

			for (auto procedure : procedures) {
				if (procedure->getLexicalLevel()) {
					auto newNode = make_shared<ScopeNode>(procedure);
					while (!pcodeStack.empty() && newNode->getLexicalLevel() < pcodeStack.top()->getLexicalLevel()) {
						newNode->add(pcodeStack.top());
						pcodeStack.pop();
					}
					pcodeStack.push(newNode);
				} else {
					nativeStack.push(make_shared<ScopeNode>(procedure));
				}
			}

			if (pcodeStack.empty()) {
				return shared_ptr<ScopeNode>();
			} else {
				while (!nativeStack.empty()) {
					pcodeStack.top()->add(nativeStack.top());
					nativeStack.pop();
				}
				return  pcodeStack.top();
			}
		}
	}

}
