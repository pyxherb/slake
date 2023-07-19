#ifndef _SLKBCC_GENERIC_H_
#define _SLKBCC_GENERIC_H_

#include "typename.h"
#include <slake/valdef/generic.h>

namespace slake {
	namespace bcc {
		using namespace std;

		struct GenericQualifier final {
			shared_ptr<TypeName> typeName;
			slake::GenericFilter filter;

			inline GenericQualifier(
				shared_ptr<TypeName> typeName,
				slake::GenericFilter filter)
				: typeName(typeName),
				  filter(filter) {
			}
		};

		struct GenericParam {
			string name;
			deque<GenericQualifier> qualifiers;
		};
	}
}

#endif
