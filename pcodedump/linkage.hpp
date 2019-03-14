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
#include <boost/endian/arithmetic.hpp>

namespace pcodedump {

	enum class LinkageType { eofMark, unitRef, globRef, publRef, privRef, constRef, globDef, publDef, constDef, extProc, extFunc, sepProc, sepFunc, seppRef, sepfRef };

	class LinkRecord {
	protected:
		LinkRecord(std::wstring name, std::uint8_t const * fieldStart);

	public:
		virtual ~LinkRecord() = 0;
		virtual bool endOfLinkage() const { return false; }
		virtual std::uint8_t const * end() const;
		virtual LinkageType linkRecordType() const = 0;
		virtual void writeOut(std::wostream & os) const;

	private:
		std::wstring name;
		std::uint8_t const * fieldStart;
	};

	class LinkReference : public LinkRecord {
	protected:
		struct Fields;

	protected:
		LinkReference(std::wstring name, std::uint8_t const * fieldStart);

	public:
		virtual ~LinkReference() = 0;
		std::uint8_t const * end() const override final;
		void writeOut(std::wostream & os) const override;
		void writeReferences(std::wostream & os) const;

	private:
		std::vector<int> extractReferences();

	protected:
		Fields const & fields;

	private:
		std::vector<int> const references;
	};

	class GlobalReference final : public LinkReference {
	public:
		GlobalReference(std::wstring name, std::uint8_t const * fieldStart) : LinkReference(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::globRef; }
	};

	class PublicReference final : public LinkReference {
	public:
		PublicReference(std::wstring name, std::uint8_t const * fieldStart) : LinkReference(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::publRef; }
	};

	class PrivateReference final : public LinkReference {
	public:
		PrivateReference(std::wstring name, std::uint8_t const * fieldStart) : LinkReference(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::privRef; }
		void writeOut(std::wostream & os) const override;
	};

	class GlobalDefinition final : public LinkRecord {
	private:
		struct Fields;

	public:
		GlobalDefinition(std::wstring name, std::uint8_t const * fieldStart);
		LinkageType linkRecordType() const override { return LinkageType::globDef; }
		void writeOut(std::wostream & os) const override final;

	private:
		Fields const & fields;
	};

	class PublicDefinition final : public LinkRecord {
	private:
		struct Fields;

	public:
		PublicDefinition(std::wstring name, std::uint8_t const * fieldStart);
		LinkageType linkRecordType() const override { return LinkageType::publDef; }
		void writeOut(std::wostream & os) const override final;

	private:
		Fields const & fields;
	};

	class ConstantDefinition final : public LinkRecord {
	private:
		struct Fields;

	public:
		ConstantDefinition(std::wstring name, std::uint8_t const * fieldStart);
		LinkageType linkRecordType() const override { return LinkageType::constDef; }
		void writeOut(std::wostream & os) const override final;

	private:
		Fields const & fields;
	};

	class LinkRoutine : public LinkRecord {
	private:
		struct Fields;

	protected:
		LinkRoutine(std::wstring name, std::uint8_t const * fieldStart);

	public:
		virtual ~LinkRoutine() = 0;
		void writeOut(std::wostream & os) const override final;

	private:
		Fields const & fields;
	};

	class ExternalProcedure final : public LinkRoutine {
	public:
		ExternalProcedure(std::wstring name, std::uint8_t const * fieldStart) : LinkRoutine(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::extProc; }
	};

	class ExternalFunction final : public LinkRoutine {
	public:
		ExternalFunction(std::wstring name, std::uint8_t const * fieldStart) : LinkRoutine(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::extFunc; }
	};

	class SeparateProcedure final : public LinkRoutine {
	public:
		SeparateProcedure(std::wstring name, std::uint8_t const * fieldStart) : LinkRoutine(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::sepProc; }
	};

	class SeparateFunction final : public LinkRoutine {
	public:
		SeparateFunction(std::wstring name, std::uint8_t const * fieldStart) : LinkRoutine(name, fieldStart) {}
		LinkageType linkRecordType() const override { return LinkageType::sepFunc; }
	};

	enum class SegmentKind;

	class EndOfFileMark final : public LinkRecord {
	private:
		struct Fields;

	public:
		EndOfFileMark(std::wstring name, SegmentKind const segmentKind, std::uint8_t const * fieldStart);
		bool endOfLinkage() const override { return true; }
		LinkageType linkRecordType() const override { return LinkageType::eofMark; }
		void writeOut(std::wostream & os) const override final;

	private:
		Fields const & fields;
		SegmentKind const segmentKind;
	};

	class CodeSegment;

	class LinkageInfo {
	public:
		LinkageInfo(CodeSegment & segment, const std::uint8_t * linkage);

		void write(std::wostream& os) const;

	private:
		std::vector<std::shared_ptr<LinkRecord>> linkRecords;
	};

}

#endif // !_424F2F1F_FA89_49E3_AC30_FD9BB48B47D3
