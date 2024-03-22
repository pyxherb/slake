#ifndef _SLKC_COMPILER_AST_CLASS_H_
#define _SLKC_COMPILER_AST_CLASS_H_

#include "member.h"
#include "scope.h"
#include "ref.h"
#include "generic.h"

namespace slake {
	namespace slkc {
		class ClassNode : public MemberNode {
		private:
			Location _loc;

		public:
			string name;
			shared_ptr<CustomTypeNameNode> parentClass;			   // Parent class
			deque<shared_ptr<CustomTypeNameNode>> implInterfaces;  // Implemented interfaces

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			shared_ptr<Scope> scope = make_shared<Scope>();

			ClassNode() = default;
			inline ClassNode(
				Location loc,
				Compiler *compiler,
				string name,
				shared_ptr<CustomTypeNameNode> parentClass,
				deque<shared_ptr<CustomTypeNameNode>> implInterfaces,
				GenericParamNodeList genericParams)
				: MemberNode(compiler, 0),
				  _loc(loc),
				  name(name),
				  parentClass(parentClass),
				  implInterfaces(implInterfaces),
				  genericParams(genericParams),
				  genericParamIndices(genGenericParamIndicies(genericParams)) {
				scope->owner = this;
			}
			virtual ~ClassNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Class; }

			virtual RefEntry getName() const override { return RefEntry(_loc, name, genericArgs); }

			ClassNode &operator=(const ClassNode &rhs) = default;
			virtual inline shared_ptr<AstNode> duplicate() override {
				shared_ptr<ClassNode> newInstance = make_shared<ClassNode>();
				(*newInstance.get()) = *this;
				return static_pointer_cast<AstNode>(newInstance);
			}
		};
	}
}

#endif
