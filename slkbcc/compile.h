#ifndef _SLKBCC_COMPILE_H_
#define _SLKBCC_COMPILE_H_

#include "scope.h"

namespace slake {
	namespace bcc {
		void compile(std::ostream &fs);
		void compileScope(std::ostream &fs, shared_ptr<Scope> scope);
		void compileOperand(std::ostream &fs, shared_ptr<Operand> operand);
		void compileRef(std::ostream &fs, shared_ptr<Ref> ref);
		void compileTypeName(std::ostream &fs, shared_ptr<TypeName> typeName);
	}
}

#endif
