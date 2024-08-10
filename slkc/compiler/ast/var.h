#ifndef _SLKC_COMPILER_AST_VAR_H_
#define _SLKC_COMPILER_AST_VAR_H_

#include "member.h"
#include "expr.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		class VarNode : public MemberNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::shared_ptr<TypeNameNode> type;
			std::string name;
			std::shared_ptr<ExprNode> initValue;
			bool isProperty;

			size_t idxNameToken = SIZE_MAX,
				   idxColonToken = SIZE_MAX,
				   idxAssignOpToken = SIZE_MAX,
				   idxCommaToken = SIZE_MAX;

			inline VarNode(const VarNode &other) : MemberNode(other) {
				if (other.type)
					type = other.type->duplicate<TypeNameNode>();
				name = other.name;
				initValue = other.initValue;
				isProperty = other.isProperty;

				idxNameToken = other.idxNameToken;
				idxColonToken = other.idxColonToken;
				idxAssignOpToken = other.idxAssignOpToken;
				idxCommaToken = other.idxCommaToken;
			}
			inline VarNode(
				Compiler *compiler,
				AccessModifier access,
				std::shared_ptr<TypeNameNode> type,
				std::string name,
				std::shared_ptr<ExprNode> initValue,
				size_t idxNameToken,
				size_t idxColonToken,
				size_t idxAssignOpToken,
				size_t idxCommaToken,
				bool isProperty = false)
				: MemberNode(compiler, access),
				  type(type),
				  name(name),
				  initValue(initValue),
				  idxNameToken(idxNameToken),
				  idxColonToken(idxColonToken),
				  idxAssignOpToken(idxAssignOpToken),
				  idxCommaToken(idxCommaToken),
				  isProperty(isProperty) {
			}
			virtual ~VarNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Var; }

			virtual IdRefEntry getName() const override { return IdRefEntry(sourceLocation, SIZE_MAX, name, genericArgs); }
		};

		class LocalVarNode : public AstNode {
		public:
			std::string name;
			uint32_t index;
			std::shared_ptr<TypeNameNode> type;

			inline LocalVarNode(std::string name, uint32_t index, std::shared_ptr<TypeNameNode> type)
				: name(name), index(index), type(type) {}
			virtual ~LocalVarNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::LocalVar; }
		};
	}
}

#endif
