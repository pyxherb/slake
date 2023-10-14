#ifndef _SLKC_COMPILER_AST_MODULE_H_
#define _SLKC_COMPILER_AST_MODULE_H_

#include "ref.h"
#include "scope.h"

namespace slake {
	namespace slkc {
		class ModuleNode : public AstNode {
		private:
			Location _loc;

		public:
			Ref moduleName;
			shared_ptr<Scope> scope;
			unordered_map<string, Ref> imports;
			weak_ptr<ModuleNode> parentModule;

			inline ModuleNode(
				Location loc,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: _loc(loc), scope(scope) {}
			virtual ~ModuleNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_MODULE; }
		};
	}
}

#endif
