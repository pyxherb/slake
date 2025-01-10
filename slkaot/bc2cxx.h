#ifndef _SLKAOT_BC2CXX_H_
#define _SLKAOT_BC2CXX_H_

#include "cxxast.h"

namespace slake {
	namespace slkaot {
		namespace bc2cxx {
			class BC2CXX {
			public:
				std::shared_ptr<cxxast::Namespace> rootNamespace;

				void compileModule(ModuleObject *moduleObject);
			};
		}
	}
}

#endif
