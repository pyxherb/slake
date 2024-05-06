#ifndef _SLKC_COMPILER_AST_MODULE_H_
#define _SLKC_COMPILER_AST_MODULE_H_

#include "ref.h"
#include "scope.h"
#include "member.h"

namespace slake {
	namespace slkc {
		struct ImportItem {
			Ref ref;
			size_t idxNameToken;
		};

		class ModuleNode : public MemberNode {
		private:
			Location _loc;

		public:
			Ref moduleName;
			unordered_map<string, ImportItem> imports;
			deque<ImportItem> unnamedImports;
			weak_ptr<ModuleNode> parentModule;

			inline ModuleNode(
				Compiler *compiler,
				Location loc,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: MemberNode(compiler, ACCESS_PUB), _loc(loc) {
				scope->owner = this;
				setScope(scope);
			}
			virtual ~ModuleNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Module; }

			virtual inline RefEntry getName() const override {
				return moduleName.back();
			}
		};
	}
}

#endif
