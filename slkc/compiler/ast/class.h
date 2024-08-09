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
			std::deque<size_t> idxImplInterfacesSeparatorTokens;

			size_t idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			std::string documentation;

			ClassNode() = default;
			inline ClassNode(const ClassNode &other) : MemberNode(other) {
				name = other.name;

				if (other.parentClass)
					parentClass = other.parentClass->duplicate<TypeNameNode>();

				implInterfaces.resize(other.implInterfaces.size());
				for (size_t i = 0; i < other.implInterfaces.size(); ++i)
					implInterfaces[i] = other.implInterfaces[i]->duplicate<TypeNameNode>();

				idxClassToken = other.idxClassToken;
				idxNameToken = other.idxNameToken;

				idxParentSlotLParentheseToken = other.idxParentSlotLParentheseToken;
				idxParentSlotRParentheseToken = other.idxParentSlotRParentheseToken;

				idxImplInterfacesColonToken = other.idxImplInterfacesColonToken;
				idxImplInterfacesSeparatorTokens = other.idxImplInterfacesSeparatorTokens;

				idxLBraceToken = other.idxLBraceToken;
				idxRBraceToken = other.idxRBraceToken;

				documentation = other.documentation;
			}
			inline ClassNode(
				Compiler *compiler,
				std::string name)
				: MemberNode(compiler, 0),
				  name(name) {
				setScope(std::make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~ClassNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Class; }

			virtual IdRefEntry getName() const override {
				if (genericArgs.size())
					return IdRefEntry(sourceLocation, SIZE_MAX, name, genericArgs);
				return IdRefEntry(sourceLocation, SIZE_MAX, name, getPlaceholderGenericArgs());
			}
		};

		class InterfaceNode : public MemberNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::string name;

			std::deque<std::shared_ptr<TypeNameNode>> parentInterfaces;	 // Parent interfaces

			size_t idxInterfaceToken = SIZE_MAX,
				   idxNameToken = SIZE_MAX;

			size_t idxImplInterfacesColonToken = SIZE_MAX;
			std::deque<size_t> idxImplInterfacesSeparatorTokens;

			size_t idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			InterfaceNode() = default;
			inline InterfaceNode(const InterfaceNode &other) : MemberNode(other) {
				name = other.name;

				parentInterfaces.resize(other.parentInterfaces.size());
				for (size_t i = 0; i < other.parentInterfaces.size(); ++i)
					parentInterfaces[i] = other.parentInterfaces[i]->duplicate<TypeNameNode>();

				idxInterfaceToken = other.idxInterfaceToken;
				idxNameToken = other.idxNameToken;

				idxImplInterfacesColonToken = other.idxImplInterfacesColonToken;
				idxImplInterfacesSeparatorTokens = other.idxImplInterfacesSeparatorTokens;

				idxLBraceToken = other.idxLBraceToken;
				idxRBraceToken = other.idxRBraceToken;
			}
			inline InterfaceNode(
				std::string name)
				: name(name) {
				setScope(std::make_shared<Scope>());
			}
			virtual ~InterfaceNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Interface; }

			virtual IdRefEntry getName() const override {
				if (genericArgs.size())
					return IdRefEntry(sourceLocation, SIZE_MAX, name, genericArgs);
				return IdRefEntry(sourceLocation, SIZE_MAX, name, getPlaceholderGenericArgs());
			}
		};
	}
}

#endif
