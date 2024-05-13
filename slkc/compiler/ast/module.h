#ifndef _SLKC_COMPILER_AST_MODULE_H_
#define _SLKC_COMPILER_AST_MODULE_H_

#include "idref.h"
#include "scope.h"
#include "member.h"

namespace slake {
	namespace slkc {
		struct ImportItem {
			IdRef ref;
			size_t idxNameToken;
		};

		class ModuleNode : public MemberNode {
		private:
			Location _loc;

		public:
			IdRef moduleName;
			std::unordered_map<std::string, ImportItem> imports;
			std::deque<ImportItem> unnamedImports;
			std::weak_ptr<ModuleNode> parentModule;

			inline ModuleNode(
				Compiler *compiler,
				Location loc,
				std::shared_ptr<Scope> scope = std::make_shared<Scope>())
				: MemberNode(compiler, ACCESS_PUB), _loc(loc) {
				scope->owner = this;
				setScope(scope);
			}
			virtual ~ModuleNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Module; }

			virtual inline IdRefEntry getName() const override {
				return moduleName.back();
			}
		};
	}
}

#endif
