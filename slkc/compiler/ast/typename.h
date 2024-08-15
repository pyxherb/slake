#ifndef _SLKC_COMPILER_AST_TYPENAME_H_
#define _SLKC_COMPILER_AST_TYPENAME_H_

#include <cstdint>
#include <variant>

#include "idref.h"
#include "scope.h"

namespace slake {
	namespace slkc {
		enum class TypeId : uint8_t {
			I8,
			I16,
			I32,
			I64,

			U8,
			U16,
			U32,
			U64,

			F32,
			F64,

			String,

			Bool,

			Auto,
			Void,

			Any,

			Array,
			Fn,

			Custom,

			Context,

			Bad
		};

		/// @brief Base type name node class.
		/// @note Duplication is unneeded, because type names should not be modified after they created.
		class TypeNameNode : public AstNode {
		public:
			bool isRef = false;
			size_t idxRefIndicatorToken = SIZE_MAX;  // Reference indicator token.

			inline TypeNameNode(const TypeNameNode &other) : AstNode(other) {
				isRef = other.isRef;
				idxRefIndicatorToken = other.idxRefIndicatorToken;
			}
			inline TypeNameNode(bool isRef = false)
				: isRef(isRef) {}
			virtual ~TypeNameNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::TypeName; }

			virtual inline TypeId getTypeId() const = 0;
		};

		class BasicSimpleTypeNameNode : public TypeNameNode {
		public:
			size_t idxToken;

			inline BasicSimpleTypeNameNode(const BasicSimpleTypeNameNode &other)
				: TypeNameNode(other), idxToken(other.idxToken) {
			}
			inline BasicSimpleTypeNameNode(size_t idxToken, bool isRef = false)
				: TypeNameNode(isRef), idxToken(idxToken) {}
			virtual ~BasicSimpleTypeNameNode() = default;
		};

		template <TypeId TID>
		class SimpleTypeNameNode : public BasicSimpleTypeNameNode {
		private:
			virtual inline std::shared_ptr<AstNode> doDuplicate() override {
				return std::make_shared<SimpleTypeNameNode>(*this);
			}

		public:
			inline SimpleTypeNameNode(const SimpleTypeNameNode<TID> &other)
				: BasicSimpleTypeNameNode(other) {
			}
			inline SimpleTypeNameNode(size_t idxToken, bool isRef = false)
				: BasicSimpleTypeNameNode(idxToken, isRef) {}
			virtual ~SimpleTypeNameNode() = default;

			virtual inline TypeId getTypeId() const override { return TID; }
		};

		using I8TypeNameNode = SimpleTypeNameNode<TypeId::I8>;
		using I16TypeNameNode = SimpleTypeNameNode<TypeId::I16>;
		using I32TypeNameNode = SimpleTypeNameNode<TypeId::I32>;
		using I64TypeNameNode = SimpleTypeNameNode<TypeId::I64>;
		using U8TypeNameNode = SimpleTypeNameNode<TypeId::U8>;
		using U16TypeNameNode = SimpleTypeNameNode<TypeId::U16>;
		using U32TypeNameNode = SimpleTypeNameNode<TypeId::U32>;
		using U64TypeNameNode = SimpleTypeNameNode<TypeId::U64>;
		using F32TypeNameNode = SimpleTypeNameNode<TypeId::F32>;
		using F64TypeNameNode = SimpleTypeNameNode<TypeId::F64>;
		using StringTypeNameNode = SimpleTypeNameNode<TypeId::String>;
		using BoolTypeNameNode = SimpleTypeNameNode<TypeId::Bool>;
		using AutoTypeNameNode = SimpleTypeNameNode<TypeId::Auto>;
		using VoidTypeNameNode = SimpleTypeNameNode<TypeId::Void>;
		using AnyTypeNameNode = SimpleTypeNameNode<TypeId::Any>;

		class CustomTypeNameNode : public TypeNameNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			IdRef ref;
			std::weak_ptr<AstNode> cachedResolvedResult;

			Compiler *compiler;
			Scope *scope;

			// bool resolved = false;

			inline CustomTypeNameNode(const CustomTypeNameNode &other)
				: TypeNameNode(other),
				  ref(duplicateIdRef(other.ref)),
				  compiler(other.compiler),
				  scope(other.scope) {
			}
			inline CustomTypeNameNode(IdRef ref, Compiler *compiler, Scope *scope, bool isRef = false)
				: TypeNameNode(isRef), ref(ref), compiler(compiler), scope(scope) {}
			virtual ~CustomTypeNameNode() = default;

			virtual inline TypeId getTypeId() const override { return TypeId::Custom; }
		};

		class ArrayTypeNameNode : public TypeNameNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::shared_ptr<TypeNameNode> elementType;
			uint32_t nDimensions = 1;

			size_t idxLBracketToken = SIZE_MAX,
				   idxRBracketToken = SIZE_MAX;

			inline ArrayTypeNameNode(const ArrayTypeNameNode &other)
				: TypeNameNode(other) {
				elementType = other.elementType->duplicate<TypeNameNode>();
				idxLBracketToken = other.idxLBracketToken;
				idxRBracketToken = other.idxRBracketToken;
			}
			inline ArrayTypeNameNode(std::shared_ptr<TypeNameNode> elementType, bool isRef = false)
				: TypeNameNode(isRef), elementType(elementType) {}
			virtual ~ArrayTypeNameNode() = default;

			virtual inline TypeId getTypeId() const override { return TypeId::Array; }
		};

		class FnTypeNameNode : public TypeNameNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::shared_ptr<TypeNameNode> returnType;
			std::deque<std::shared_ptr<TypeNameNode>> paramTypes;

			inline FnTypeNameNode(const FnTypeNameNode &other)
				: TypeNameNode(other), returnType(other.returnType->duplicate<TypeNameNode>()) {
				paramTypes.resize(other.paramTypes.size());

				for (size_t i = 0; i < other.paramTypes.size(); ++i)
					paramTypes[i] = other.paramTypes[i]->duplicate<TypeNameNode>();
			}
			inline FnTypeNameNode(
				std::shared_ptr<TypeNameNode> returnType,
				std::deque<std::shared_ptr<TypeNameNode>> paramTypes,
				bool isRef = false)
				: TypeNameNode(isRef),
				  returnType(returnType),
				  paramTypes(paramTypes) {}
			virtual ~FnTypeNameNode() = default;

			virtual inline TypeId getTypeId() const override { return TypeId::Fn; }
		};

		class ContextTypeNameNode : public TypeNameNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::shared_ptr<TypeNameNode> resultType;

			size_t idxIndicatorToken = SIZE_MAX;

			inline ContextTypeNameNode(const ContextTypeNameNode &other) : TypeNameNode(other), resultType(other.resultType) {}
			inline ContextTypeNameNode(
				std::shared_ptr<TypeNameNode> resultType)
				: TypeNameNode(false) {}
			virtual ~ContextTypeNameNode() = default;

			virtual inline TypeId getTypeId() const override { return TypeId::Context; }
		};

		class BadTypeNameNode : public TypeNameNode {
		private:
			virtual inline std::shared_ptr<AstNode> doDuplicate() override {
				return std::make_shared<BadTypeNameNode>(*this);
			}

		public:
			size_t idxStartToken = SIZE_MAX, idxEndToken = SIZE_MAX;

			inline BadTypeNameNode(const BadTypeNameNode &other)
				: TypeNameNode(other.isRef) {
				idxStartToken = other.idxStartToken;
				idxEndToken = other.idxEndToken;
			}
			inline BadTypeNameNode(size_t idxStartToken, size_t idxEndToken, bool isRef = false)
				: TypeNameNode(isRef), idxStartToken(idxStartToken), idxEndToken(idxEndToken) {}
			virtual ~BadTypeNameNode() = default;

			virtual inline TypeId getTypeId() const override { return TypeId::Bad; }
		};

		class Compiler;
	}
}

namespace std {
	std::string to_string(std::shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool forMangling = false);
}

#endif
