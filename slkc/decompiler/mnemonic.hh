#ifndef _SLKC_DECOMPILER_MNEMONIC_HH_
#define _SLKC_DECOMPILER_MNEMONIC_HH_

#include <unordered_map>
#include <slake/opcode.h>

namespace Slake {
	namespace Decompiler {
		extern std::unordered_map<Opcode, const char*> mnemonics;
	}
}

#endif
