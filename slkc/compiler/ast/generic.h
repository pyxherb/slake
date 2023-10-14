#ifndef _SLKC_COMPILER_AST_GENERIC_H_
#define _SLKC_COMPILER_AST_GENERIC_H_

#include "typename.h"
#include <slake/access.h>
#include <string>

namespace slake {
	namespace slkc {
		enum GenericFilter : uint8_t {
			GFLT_EXTENDS = 0,
			GFLT_IMPLS,
			GFLT_CONSISTSOF
		};

		struct GenericQualifier {
			GenericFilter filter;
			shared_ptr<TypeNameNode> type;

			inline GenericQualifier(GenericFilter filter, shared_ptr<TypeNameNode> type = {})
				: filter(filter), type(type) {}
		};

		struct GenericParam {
			string name;
			deque<GenericQualifier> qualifiers;

			inline GenericParam(string name, std::deque<GenericQualifier> qualifiers)
				: name(name), qualifiers(qualifiers) {}
		};
	}
}

#endif
