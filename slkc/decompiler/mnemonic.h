#ifndef _SLKC_DECOMPILER_MNEMONIC_HH_
#define _SLKC_DECOMPILER_MNEMONIC_HH_

#include <unordered_map>
#include <slake/opcode.h>
#include <string>

namespace slake {
	namespace Decompiler {
		using namespace std;
		extern map<Opcode, std::string> mnemonics;
	}
}

#endif
