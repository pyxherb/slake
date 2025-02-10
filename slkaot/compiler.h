#ifndef _SLKAOT_COMPILER_H_
#define _SLKAOT_COMPILER_H_

#include "bc2cxx.h"
#include <fstream>

namespace slake {
	namespace slkaot {
		extern std::vector<std::string> modulePaths;

		std::unique_ptr<std::ifstream> moduleLocator(Runtime *rt, HostObjectRef<IdRefObject> ref);
	}
}

#endif
