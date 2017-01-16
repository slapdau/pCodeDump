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
	extern std::string filename;
	extern cpu_t cpu;

	bool parseOptions(int argc, char *argv[], char *envp[]);

}

#endif // !_7A0EDA10_B113_4733_8A7C_0F131220A28C
