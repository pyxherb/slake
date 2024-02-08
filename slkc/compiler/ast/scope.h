///
/// @file scope.h
/// @brief Header file of the scope module.
///
/// @copyright Copyright (c) 2023 Slake contributors
///
///
#ifndef _SLKC_COMPILER_AST_SCOPE_H_
#define _SLKC_COMPILER_AST_SCOPE_H_

#include "astnode.h"

namespace slake {
	namespace slkc {
		class MemberNode;

		class Scope {
		public:
			AstNode *owner = nullptr;								// Owner of this scope.
			Scope *parent = nullptr;								// Parent scope.
			unordered_map<string, shared_ptr<MemberNode>> members;	// Members owned by this scope.
		};
	}
}

#endif
