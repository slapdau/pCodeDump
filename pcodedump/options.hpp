#ifndef _7A0EDA10_B113_4733_8A7C_0F131220A28C
#define _7A0EDA10_B113_4733_8A7C_0F131220A28C

#include <string>

namespace pcodedump {

	enum class cpu_t { _6502, _65c02, _65c816 };

	extern bool showLinkage;
	extern bool showText;
	extern bool listProcs;
	extern bool addressOrder;
	extern bool disasmProcs;
	extern std::wstring filename;
	extern cpu_t cpu;

	bool parseOptions(int argc, wchar_t *argv[], wchar_t *envp[]);

}

#endif // !_7A0EDA10_B113_4733_8A7C_0F131220A28C
