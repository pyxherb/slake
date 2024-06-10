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
		public:
			IdRef moduleName;
			std::unordered_map<std::string, ImportItem> imports;
			std::deque<ImportItem> unnamedImports;
			std::weak_ptr<ModuleNode> parentModule;

			inline ModuleNode(
				Compiler *compiler,
				std::shared_ptr<Scope> scope = std::make_shared<Scope>())
				: MemberNode(compiler, ACCESS_PUB) {
				scope->owner = this;
				setScope(scope);
			}
			virtual ~ModuleNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Module; }

			virtual inline IdRefEntry getName() const override {
				return moduleName.back();
			}
		};
	}
}

#endif
