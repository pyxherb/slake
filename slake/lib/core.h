#ifndef _SLAKE_LIB_CORE_H_
#define _SLAKE_LIB_CORE_H_

#include <slake/runtime.h>

namespace slake {
	namespace stdlib {
		namespace Core {
			namespace Except {
				extern ModuleValue *modExcept;
				extern InterfaceValue *typeIException;
				extern ClassValue *exLogicalError;
				extern ClassValue *exDivideByZeroError;
				extern ClassValue *exOutOfMemoryError;
				extern ClassValue *exInvalidOpcodeError;
				extern ClassValue *exInvalidOperandsError;

				void load(Runtime *rt);
			}
			extern ModuleValue *modCore;

			void load(Runtime *rt);
		}
	}
}

#endif
