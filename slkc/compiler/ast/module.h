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
			Ref moduleName;
			shared_ptr<Scope> scope;
			unordered_map<string, Ref> imports;
			weak_ptr<ModuleNode> parentModule;

			inline ModuleNode(
				Compiler *compiler,
				Location loc,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: MemberNode(compiler, ACCESS_PUB), _loc(loc), scope(scope) {
				scope->owner = this;
			}
			virtual ~ModuleNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Module; }

			virtual RefEntry getName() const override { return moduleName.back(); }
		};
	}
}

#endif
