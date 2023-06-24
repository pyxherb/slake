#ifndef _SLKC_DECOMPILER_MNEMONIC_HH_
#define _SLKC_DECOMPILER_MNEMONIC_HH_

#include <unordered_map>
#include <slake/opcode.h>

namespace Slake {
	namespace Decompiler {
		using namespace std;
		extern unordered_map<Opcode, std::string> mnemonics;
	}
}

#endif
