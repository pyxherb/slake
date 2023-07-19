#ifndef _SLKC_COMPILER_REF_H_
#define _SLKC_COMPILER_REF_H_

#include "node.h"

namespace slake {
	namespace slkc {
		class TypeName;

		struct RefEntry {
			string name;
			deque<shared_ptr<TypeName>> genericArgs;

			inline RefEntry(string name, deque<shared_ptr<TypeName>> genericArgs = {})
				: name(name), genericArgs(genericArgs) {}
		};

		using Ref = deque<RefEntry>;
	}
}

#endif
