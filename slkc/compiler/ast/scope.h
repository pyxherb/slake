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

		class Scope : public std::enable_shared_from_this<Scope> {
		public:
			AstNode *owner = nullptr;											   // Owner of this scope.
			Scope *parent = nullptr;											   // Parent of this scope, owned by the owner's owner.
			std::unordered_map<std::string, std::shared_ptr<MemberNode>> members;  // Members owned by this scope.

			Scope *duplicate();

			void setOwner(MemberNode *owner);
		};

		MemberNode *parentOf(MemberNode *node);
	}
}

#endif
