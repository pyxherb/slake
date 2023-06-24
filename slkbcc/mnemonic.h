#ifndef _SLKBCC_MNEMONIC_HH_
#define _SLKBCC_MNEMONIC_HH_

#include <unordered_map>
#include <slake/opcode.h>

namespace Slake {
	namespace Assembler {
		using namespace std;
		extern unordered_map<std::string, Opcode> mnemonics;
	}
}

#endif
