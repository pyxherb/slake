#ifndef _SLKC_COMPILER_AST_CLASS_H_
#define _SLKC_COMPILER_AST_CLASS_H_

#include "member.h"
#include "scope.h"
#include "idref.h"
#include "generic.h"

namespace slake {
	namespace slkc {
		struct ParentSlot {
			std::shared_ptr<TypeNameNode> typeName;

			size_t idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;
		};

		class ClassNode : public MemberNode {
		private:
			Location _loc;

			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::string name;
			std::shared_ptr<TypeNameNode> parentClass;				   // Parent class
			std::deque<std::shared_ptr<TypeNameNode>> implInterfaces;  // Implemented interfaces

			size_t idxClassToken = SIZE_MAX,
				   idxNameToken = SIZE_MAX;

			size_t idxParentSlotLParentheseToken = SIZE_MAX,
				   idxParentSlotRParentheseToken = SIZE_MAX;

			size_t idxImplInterfacesColonToken = SIZE_MAX;
			std::deque<size_t> idxImplInterfacesCommaTokens;

			size_t idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			ClassNode() = default;
			inline ClassNode(const ClassNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				if (parentClass)
					parentClass = other.parentClass->duplicate<TypeNameNode>();

				implInterfaces.resize(other.implInterfaces.size());
				for (size_t i = 0; i < other.implInterfaces.size(); ++i)
					implInterfaces[i] = other.implInterfaces[i]->duplicate<TypeNameNode>();

				idxClassToken = other.idxClassToken;
				idxNameToken = other.idxNameToken;

				idxParentSlotLParentheseToken = other.idxParentSlotLParentheseToken;
				idxParentSlotRParentheseToken = other.idxParentSlotRParentheseToken;

				idxImplInterfacesColonToken = other.idxImplInterfacesColonToken;
				idxImplInterfacesCommaTokens = other.idxImplInterfacesCommaTokens;

				idxLBraceToken = other.idxLBraceToken;
				idxRBraceToken = other.idxRBraceToken;
			}
			inline ClassNode(
				Location loc,
				Compiler *compiler,
				std::string name)
				: MemberNode(compiler, 0),
				  _loc(loc),
				  name(name) {
				setScope(std::make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~ClassNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Class; }

			virtual IdRefEntry getName() const override {
				if (genericArgs.size())
					return IdRefEntry(_loc, SIZE_MAX, name, genericArgs);
				return IdRefEntry(_loc, SIZE_MAX, name, getPlaceholderGenericArgs());
			}
		};

		class InterfaceNode : public MemberNode {
		private:
			Location _loc;

			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::string name;

			std::deque<std::shared_ptr<TypeNameNode>> parentInterfaces;	 // Parent interfaces

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			size_t idxInterfaceToken = SIZE_MAX,
				   idxNameToken = SIZE_MAX;

			size_t idxImplInterfacesColonToken = SIZE_MAX;
			std::deque<size_t> idxImplInterfacesCommaTokens;

			size_t idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			InterfaceNode() = default;
			inline InterfaceNode(const InterfaceNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				parentInterfaces.resize(other.parentInterfaces.size());
				for (size_t i = 0; i < other.parentInterfaces.size(); ++i)
					parentInterfaces[i] = other.parentInterfaces[i]->duplicate<TypeNameNode>();

				idxInterfaceToken = other.idxInterfaceToken;
				idxNameToken = other.idxNameToken;

				idxImplInterfacesColonToken = other.idxImplInterfacesColonToken;
				idxImplInterfacesCommaTokens = other.idxImplInterfacesCommaTokens;

				idxLBraceToken = other.idxLBraceToken;
				idxRBraceToken = other.idxRBraceToken;
			}
			inline InterfaceNode(
				Location loc,
				std::string name)
				: _loc(loc),
				  name(name) {
				setScope(std::make_shared<Scope>());
			}
			virtual ~InterfaceNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Interface; }

			virtual IdRefEntry getName() const override {
				if (genericArgs.size())
					return IdRefEntry(_loc, SIZE_MAX, name, genericArgs);
				return IdRefEntry(_loc, SIZE_MAX, name, getPlaceholderGenericArgs());
			}
		};

		class TraitNode : public MemberNode {
		private:
			Location _loc;

			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::string name;

			std::deque<std::shared_ptr<TypeNameNode>> parentTraits;	 // Parent traits

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			std::shared_ptr<Scope> scope = std::make_shared<Scope>();

			size_t idxTraitToken = SIZE_MAX,
				   idxNameToken = SIZE_MAX;

			size_t idxImplTraitsColonToken = SIZE_MAX;
			std::deque<size_t> idxImplTraitsCommaTokens;

			size_t idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			TraitNode() = default;
			inline TraitNode(const TraitNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				parentTraits.resize(other.parentTraits.size());
				for (size_t i = 0; i < other.parentTraits.size(); ++i)
					parentTraits[i] = other.parentTraits[i]->duplicate<TypeNameNode>();

				idxTraitToken = other.idxTraitToken;
				idxNameToken = other.idxNameToken;

				idxImplTraitsColonToken = other.idxImplTraitsColonToken;
				idxImplTraitsCommaTokens = other.idxImplTraitsCommaTokens;

				idxLBraceToken = other.idxLBraceToken;
				idxRBraceToken = other.idxRBraceToken;
			}
			inline TraitNode(
				Location loc,
				std::string name)
				: _loc(loc),
				  name(name) {
				setScope(std::make_shared<Scope>());
			}
			virtual ~TraitNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Trait; }

			virtual IdRefEntry getName() const override {
				if (genericArgs.size())
					return IdRefEntry(_loc, SIZE_MAX, name, genericArgs);
				return IdRefEntry(_loc, SIZE_MAX, name, getPlaceholderGenericArgs());
			}
		};
	}
}

#endif
