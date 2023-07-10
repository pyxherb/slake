#ifndef _SLKC_COMPILER_MODULE_H_
#define _SLKC_COMPILER_MODULE_H_

#include "ref.h"

namespace slake {
	namespace slkc {
		class ModuleNode : public AstNode {
		public:
			shared_ptr<Ref> moduleName;

			virtual ~ModuleNode() = default;
		};
	}
}

#endif
