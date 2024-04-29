#ifndef _SLKC_COMPILER_AST_VAR_H_
#define _SLKC_COMPILER_AST_VAR_H_

#include "member.h"
#include "expr.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		class VarNode : public MemberNode {
		private:
			Location _loc;

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			shared_ptr<TypeNameNode> type;
			string name;
			shared_ptr<ExprNode> initValue;

			size_t idxNameToken;

			inline VarNode(const VarNode &other) : MemberNode(other) {
				_loc = other._loc;

				if (type)
					type = other.type->duplicate<TypeNameNode>();
				name = other.name;
				initValue = other.initValue;

				idxNameToken = other.idxNameToken;
			}
			inline VarNode(
				Location loc,
				Compiler *compiler,
				AccessModifier access,
				shared_ptr<TypeNameNode> type,
				string name,
				shared_ptr<ExprNode> initValue,
				size_t idxNameToken)
				: MemberNode(compiler, access),
				  _loc(loc),
				  type(type),
				  name(name),
				  initValue(initValue),
				  idxNameToken(idxNameToken) {
			}
			virtual ~VarNode() = default;

			virtual inline Location getLocation() const override { return _loc; }
			virtual inline NodeType getNodeType() const override { return NodeType::Var; }

			virtual RefEntry getName() const override { return RefEntry(_loc, SIZE_MAX, name, genericArgs); }
		};

		class LocalVarNode : public AstNode {
		public:
			uint32_t index;
			shared_ptr<TypeNameNode> type;

			inline LocalVarNode(uint32_t index, shared_ptr<TypeNameNode> type)
				: index(index), type(type) {}
			virtual ~LocalVarNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Unsupported operation"); }

			virtual inline NodeType getNodeType() const override { return NodeType::LocalVar; }
		};
	}
}

#endif
