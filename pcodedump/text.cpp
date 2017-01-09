#include "text.hpp"
#include <string>
#include <tuple>
#include <locale>

using namespace std;

namespace pcodedump {

	TextSegment::TextSegment(SegmentDirectoryEntry & directoryEntry, const uint8_t * text) :
		directoryEntry{ directoryEntry }, text{ text }
	{
	}

	namespace {

		const wstring implementation = L"IMPLEMENTATION";

		template <typename charT>
		bool compareNoCase(const charT left, const charT right) {
			locale loc{};
			return toupper<charT>(left, loc) == toupper<charT>(right, loc);
		}

		tuple<wstring, const uint8_t *> readline(const uint8_t * input) {
			wstring result{};
			uint8_t next;
			next = *input++;
			if (next == 0x10) {
				int count = (*input++) - 32;
				result.insert(end(result), count, L' ');
				next = *input++;
			}
			while (next != '\r') {
				result.push_back(next);
				if (result.size() >= implementation.size()) {
					if (equal(begin(implementation), end(implementation), end(result) - implementation.size(), compareNoCase<wchar_t>)) {
						result.erase(result.size() - implementation.size(), implementation.size());
						return make_tuple(result, nullptr);
					}
				}
				next = *input++;
			}
			return make_tuple(result, input);
		}

	}

	void TextSegment::write(std::wostream& os) const {
		auto current = text;
		while (current) {
			wstring line;
			tie(line, current) = readline(current);
			wcout << line << endl;
		}
	}

}