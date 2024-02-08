#ifndef _SLKC_COMPILER_AST_MODULE_H_
#define _SLKC_COMPILER_AST_MODULE_H_

#include "ref.h"
#include "scope.h"
#include "member.h"

namespace slake {
	namespace slkc {
		class ModuleNode : public MemberNode {
		private:
			Location _loc;

		public:
			ModuleRef moduleName;
			shared_ptr<Scope> scope;
			unordered_map<string, ModuleRef> imports;
			weak_ptr<ModuleNode> parentModule;

			inline ModuleNode(
				Location loc,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: MemberNode(ACCESS_PUB), _loc(loc), scope(scope) {
				scope->owner = this;
			}
			virtual ~ModuleNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_MODULE; }

			virtual RefEntry getName() const override { return toRegularRef(moduleName).back(); }
		};
	}
}

#endif
