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

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;
			shared_ptr<TypeNameNode> parentClass;			 // Parent class
			deque<shared_ptr<TypeNameNode>> implInterfaces;	 // Implemented interfaces

			ClassNode() = default;
			inline ClassNode(const ClassNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				if (parentClass)
					parentClass = other.parentClass->duplicate<TypeNameNode>();

				implInterfaces.resize(other.implInterfaces.size());
				for (size_t i = 0; i < other.implInterfaces.size(); ++i)
					implInterfaces[i] = other.implInterfaces[i]->duplicate<TypeNameNode>();
			}
			inline ClassNode(
				Location loc,
				Compiler *compiler,
				string name,
				shared_ptr<TypeNameNode> parentClass,
				deque<shared_ptr<TypeNameNode>> implInterfaces,
				GenericParamNodeList genericParams)
				: MemberNode(compiler, 0),
				  _loc(loc),
				  name(name),
				  parentClass(parentClass),
				  implInterfaces(implInterfaces) {
				setScope(make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~ClassNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Class; }

			virtual RefEntry getName() const override { return RefEntry(_loc, name, genericArgs); }
		};
	}
}

#endif
