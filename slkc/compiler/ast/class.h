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

			size_t idxNameToken;

			ClassNode() = default;
			inline ClassNode(const ClassNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				if (parentClass)
					parentClass = other.parentClass->duplicate<TypeNameNode>();

				implInterfaces.resize(other.implInterfaces.size());
				for (size_t i = 0; i < other.implInterfaces.size(); ++i)
					implInterfaces[i] = other.implInterfaces[i]->duplicate<TypeNameNode>();

				idxNameToken = other.idxNameToken;
			}
			inline ClassNode(
				Location loc,
				Compiler *compiler,
				string name,
				shared_ptr<TypeNameNode> parentClass,
				deque<shared_ptr<TypeNameNode>> implInterfaces,
				GenericParamNodeList genericParams,
				size_t idxNameToken)
				: MemberNode(compiler, 0),
				  _loc(loc),
				  name(name),
				  parentClass(parentClass),
				  implInterfaces(implInterfaces),
				  idxNameToken(idxNameToken) {
				setScope(make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~ClassNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Class; }

			virtual RefEntry getName() const override { return RefEntry(_loc, SIZE_MAX, name, genericArgs); }
		};

		class InterfaceNode : public MemberNode {
		private:
			Location _loc;

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;

			deque<shared_ptr<TypeNameNode>> parentInterfaces;  // Parent interfaces

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			shared_ptr<Scope> scope = make_shared<Scope>();

			size_t idxNameToken;

			InterfaceNode() = default;
			inline InterfaceNode(const InterfaceNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				parentInterfaces.resize(other.parentInterfaces.size());
				for (size_t i = 0; i < other.parentInterfaces.size(); ++i)
					parentInterfaces[i] = other.parentInterfaces[i]->duplicate<TypeNameNode>();

				idxNameToken = other.idxNameToken;
			}
			inline InterfaceNode(
				Location loc,
				string name,
				deque<shared_ptr<TypeNameNode>> parentInterfaces,
				GenericParamNodeList genericParams,
				size_t idxNameToken)
				: _loc(loc),
				  name(name),
				  parentInterfaces(parentInterfaces),
				  idxNameToken(idxNameToken) {
				scope = make_shared<Scope>();
				setScope(make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~InterfaceNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Interface; }

			virtual RefEntry getName() const override { return RefEntry(_loc, SIZE_MAX, name, genericArgs); }
		};

		class TraitNode : public MemberNode {
		private:
			Location _loc;

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;

			deque<shared_ptr<TypeNameNode>> parentTraits;  // Parent traits

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			shared_ptr<Scope> scope = make_shared<Scope>();

			size_t idxNameToken;

			TraitNode() = default;
			inline TraitNode(const TraitNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				parentTraits.resize(other.parentTraits.size());
				for (size_t i = 0; i < other.parentTraits.size(); ++i)
					parentTraits[i] = other.parentTraits[i]->duplicate<TypeNameNode>();

				idxNameToken = other.idxNameToken;
			}
			inline TraitNode(
				Location loc,
				deque<shared_ptr<TypeNameNode>> parentTraits,
				GenericParamNodeList genericParams,
				size_t idxNameToken)
				: _loc(loc),
				  parentTraits(parentTraits),
				  idxNameToken(idxNameToken) {
				setScope(make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~TraitNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Trait; }

			virtual RefEntry getName() const override { return RefEntry(_loc, SIZE_MAX, name, genericArgs); }
		};
	}
}

#endif
