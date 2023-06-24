#ifndef _SLKC_COMPILER_REF_H_
#define _SLKC_COMPILER_REF_H_

#include "node.h"

namespace Slake {
	namespace Compiler {
		class TypeName;

		struct RefScope {
			string name;
			deque<shared_ptr<TypeName>> genericArgs;

			inline RefScope(string name, deque<shared_ptr<TypeName>> genericArgs = {})
				: name(name), genericArgs(genericArgs) {}
		};

		using Ref = deque<RefScope>;
	}
}

#endif
