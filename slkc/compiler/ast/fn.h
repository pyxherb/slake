#include "typename.h"
#include <slake/access.h>

namespace Slake {
	namespace Compiler {
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
