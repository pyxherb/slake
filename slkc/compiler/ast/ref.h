#ifndef _SLKC_COMPILER_AST_REF_H_
#define _SLKC_COMPILER_AST_REF_H_

#include "astnode.h"

namespace slake {
	namespace slkc {
		class TypeNameNode;

		struct RefEntry {
			Location loc;
			string name;
			deque<shared_ptr<TypeNameNode>> genericArgs;

			inline RefEntry(Location loc, string name, deque<shared_ptr<TypeNameNode>> genericArgs = {})
				: loc(loc), name(name), genericArgs(genericArgs) {}
		};

		using Ref = deque<RefEntry>;
	}
}

namespace std {
	string to_string(const slake::slkc::Ref& ref);
}

#endif
