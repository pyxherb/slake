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

		public:
			shared_ptr<TypeNameNode> type;
			string name;
			shared_ptr<ExprNode> initValue;

			inline VarNode(
				Location loc,
				shared_ptr<TypeNameNode> type,
				string name,
				shared_ptr<ExprNode> initValue)
				: MemberNode(AST_VAR),
				  _loc(loc),
				  type(type),
				  name(name),
				  initValue(initValue) {
			}
			virtual ~VarNode() = default;

			virtual inline Location getLocation() const override { return _loc; }
		};

		class LocalVarNode : public AstNode {
		public:
			uint32_t index;
			shared_ptr<TypeNameNode> type;

			inline LocalVarNode(uint32_t index, shared_ptr<TypeNameNode> type)
				: index(index), type(type) {}
			virtual ~LocalVarNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Unsupported operation"); }

			virtual inline NodeType getNodeType() const override { return AST_LOCAL_VAR; }
		};
	}
}

#endif
