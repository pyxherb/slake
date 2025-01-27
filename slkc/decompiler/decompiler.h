#ifndef _SLKC_DECOMPILER_DECOMPILER_HH
#define _SLKC_DECOMPILER_DECOMPILER_HH

#include <slake/slxfmt.h>
#include <slake/runtime.h>
#include <slake/opti/proganal.h>

#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include "../compiler/compiler.h"

namespace slake {
	namespace decompiler {
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
		std::string decompileTypeName(const Type &type, Runtime *rt);
		std::string decompileIdRef(const IdRefObject *ref);
		void decompileObject(Runtime *rt, Object *object, std::ostream &os, int indentLevel = 0);
		void decompileValue(Runtime *rt, Value value, std::ostream &os, int indentLevel = 0);
		std::string accessToString(AccessModifier access);
	}
}

extern std::deque<std::string> modulePaths;

#endif
