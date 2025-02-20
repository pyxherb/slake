#ifndef _SLKAOT_COMPILER_H_
#define _SLKAOT_COMPILER_H_

#include "bc2cxx.h"
#include <fstream>

namespace slake {
	namespace slkaot {
		extern std::vector<std::string> modulePaths;
		extern std::string srcPath, headerOutPath, sourceOutPath, includeName;

		std::unique_ptr<std::istream> moduleLocator(Runtime *rt, const peff::DynArray<IdRefEntry> &ref);
	}
}

#endif
