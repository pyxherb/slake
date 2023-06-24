#ifndef _SLKC_DECOMPILER_DECOMPILER_HH
#define _SLKC_DECOMPILER_DECOMPILER_HH

#include <slake/slxfmt.h>
#include <slake/runtime.h>

#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace Slake {
	namespace Decompiler {
		using namespace std;

		class DecompileError : public std::runtime_error {
		public:
			inline DecompileError(std::string msg) : runtime_error(msg) {}
			virtual inline ~DecompileError() {}
		};

		enum class DecompileState {
			NORMAL = 0,
			INLINE
		};

		void decompile(std::istream &fs, std::ostream &os);
		void decompileValue(Runtime* rt, Value *value, std::ostream &os, int indentLevel = 0);
		std::string getTypeName(Runtime *rt, Type type);
		std::string refToString(shared_ptr<RefValue> ref);
		std::string accessToString(AccessModifier access);
	}
}

#endif
