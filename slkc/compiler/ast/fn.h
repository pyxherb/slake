#include "typename.h"
#include <slake/access.h>

namespace slake {
	namespace slkc {
		struct Param {
			shared_ptr<TypeName> type;
			std::string name;
			AccessModifier access;
		};

		class Fn {
		public:
		};
	}
}
