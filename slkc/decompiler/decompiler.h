#ifndef _SLKC_DECOMPILER_DECOMPILER_HH
#define _SLKC_DECOMPILER_DECOMPILER_HH

#include <slake/slxfmt.h>
#include <slake/runtime.h>

#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include "../compiler/compiler.h"

namespace slake {
	namespace decompiler {
		using namespace std;

		class DecompileError : public std::runtime_error {
		public:
			inline DecompileError(std::string msg) : runtime_error(msg) {}
			virtual ~DecompileError() = default;
		};

		enum class DecompileState {
			NORMAL = 0,
			INLINE
		};

		using DecompilerFlags = uint32_t;
		constexpr static DecompilerFlags
			DECOMP_SRCLOC = 0x00000001;

		extern DecompilerFlags decompilerFlags;

		void decompile(std::istream &fs, std::ostream &os);
		void decompileValue(Runtime* rt, Value *value, std::ostream &os, int indentLevel = 0);
		std::string accessToString(AccessModifier access);
	}
}

#endif
