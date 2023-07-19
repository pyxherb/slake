#ifndef _SLKBCC_MNEMONIC_HH_
#define _SLKBCC_MNEMONIC_HH_

#include <unordered_map>
#include <string>
#include <slake/opcode.h>

namespace slake {
	namespace bcc {
		using namespace std;
		extern unordered_map<string, Opcode> mnemonics;
	}
}

#endif
