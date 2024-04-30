#ifndef _SLKC_COMPILER_AST_TYPENAME_H_
#define _SLKC_COMPILER_AST_TYPENAME_H_

#include <cstdint>
#include <variant>

#include "ref.h"
#include "scope.h"

namespace slake {
	namespace slkc {
		enum class Type : uint8_t {
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
			WString,

			Bool,

			Auto,
			Void,

			Any,

			Array,
			Map,
			Fn,

			Custom,
			Ref,

			Bad
		};

		/// @brief Base type name node class.
		/// @note Duplication is unneeded, because type names should not be modified after they created.
		class TypeNameNode : public AstNode {
		private:
			Location _loc;

		public:
			bool isConst;  // For parameters.

			inline TypeNameNode(const TypeNameNode &other)
				: _loc(other._loc),
				  isConst(other.isConst) {
			}
			inline TypeNameNode(Location loc, bool isConst = false)
				: _loc(loc), isConst(isConst) {}
			virtual ~TypeNameNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::TypeName; }

			virtual inline Type getTypeId() const = 0;
		};

		class BasicSimpleTypeNameNode : public TypeNameNode {
		public:
			size_t idxToken;

			inline BasicSimpleTypeNameNode(const BasicSimpleTypeNameNode &other)
				: TypeNameNode(other), idxToken(other.idxToken) {
			}
			inline BasicSimpleTypeNameNode(Location loc, size_t idxToken, bool isConst = false)
				: TypeNameNode(loc, isConst), idxToken(idxToken) {}
			virtual ~BasicSimpleTypeNameNode() = default;
		};

		template <Type TID>
		class SimpleTypeNameNode : public BasicSimpleTypeNameNode {
		private:
			virtual inline shared_ptr<AstNode> doDuplicate() override {
				return make_shared<SimpleTypeNameNode>(*this);
			}

		public:
			inline SimpleTypeNameNode(const SimpleTypeNameNode<TID> &other)
				: BasicSimpleTypeNameNode(other) {
			}
			inline SimpleTypeNameNode(Location loc, size_t idxToken, bool isConst = false)
				: BasicSimpleTypeNameNode(loc, idxToken, isConst) {}
			virtual ~SimpleTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return TID; }
		};

		using I8TypeNameNode = SimpleTypeNameNode<Type::I8>;
		using I16TypeNameNode = SimpleTypeNameNode<Type::I16>;
		using I32TypeNameNode = SimpleTypeNameNode<Type::I32>;
		using I64TypeNameNode = SimpleTypeNameNode<Type::I64>;
		using U8TypeNameNode = SimpleTypeNameNode<Type::U8>;
		using U16TypeNameNode = SimpleTypeNameNode<Type::U16>;
		using U32TypeNameNode = SimpleTypeNameNode<Type::U32>;
		using U64TypeNameNode = SimpleTypeNameNode<Type::U64>;
		using F32TypeNameNode = SimpleTypeNameNode<Type::F32>;
		using F64TypeNameNode = SimpleTypeNameNode<Type::F64>;
		using StringTypeNameNode = SimpleTypeNameNode<Type::String>;
		using WStringTypeNameNode = SimpleTypeNameNode<Type::WString>;
		using BoolTypeNameNode = SimpleTypeNameNode<Type::Bool>;
		using AutoTypeNameNode = SimpleTypeNameNode<Type::Auto>;
		using VoidTypeNameNode = SimpleTypeNameNode<Type::Void>;
		using AnyTypeNameNode = SimpleTypeNameNode<Type::Any>;

		class CustomTypeNameNode : public TypeNameNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			Ref ref;
			deque<pair<Ref, shared_ptr<AstNode>>> resolvedPartsOut;

			Compiler *compiler;
			Scope *scope;

			// bool resolved = false;

			inline CustomTypeNameNode(const CustomTypeNameNode &other)
				: TypeNameNode(other),
				  ref(duplicateRef(other.ref)),
				  resolvedPartsOut(other.resolvedPartsOut),
				  compiler(other.compiler),
				  scope(other.scope) {
			}
			inline CustomTypeNameNode(Location loc, Ref ref, Compiler *compiler, Scope *scope, bool isConst = false)
				: TypeNameNode(loc, isConst), ref(ref), compiler(compiler), scope(scope) {}
			virtual ~CustomTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Custom; }
		};

		class ArrayTypeNameNode : public TypeNameNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			shared_ptr<TypeNameNode> elementType;

			inline ArrayTypeNameNode(const ArrayTypeNameNode &other)
				: TypeNameNode(other), elementType(other.elementType->duplicate<TypeNameNode>()) {}
			inline ArrayTypeNameNode(shared_ptr<TypeNameNode> elementType, bool isConst = false)
				: TypeNameNode(elementType->getLocation(), isConst), elementType(elementType) {}
			virtual ~ArrayTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Array; }
		};

		class MapTypeNameNode : public TypeNameNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			shared_ptr<TypeNameNode> keyType, valueType;

			inline MapTypeNameNode(const MapTypeNameNode &other)
				: TypeNameNode(other), keyType(other.keyType->duplicate<TypeNameNode>()), valueType(other.valueType->duplicate<TypeNameNode>()) {}
			inline MapTypeNameNode(shared_ptr<TypeNameNode> keyType, shared_ptr<TypeNameNode> valueType, bool isConst = false)
				: TypeNameNode(keyType->getLocation(), isConst), keyType(keyType), valueType(valueType) {}
			virtual ~MapTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Map; }
		};

		class FnTypeNameNode : public TypeNameNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			Location loc;
			shared_ptr<TypeNameNode> returnType;
			deque<shared_ptr<TypeNameNode>> paramTypes;

			inline FnTypeNameNode(const FnTypeNameNode &other)
				: TypeNameNode(other), returnType(other.returnType->duplicate<TypeNameNode>()) {
				paramTypes.resize(other.paramTypes.size());

				for (size_t i = 0; i < other.paramTypes.size(); ++i)
					paramTypes[i] = other.paramTypes[i]->duplicate<TypeNameNode>();
			}
			inline FnTypeNameNode(
				Location location,
				shared_ptr<TypeNameNode> returnType,
				deque<shared_ptr<TypeNameNode>> paramTypes,
				bool isConst = false)
				: TypeNameNode(location, isConst),
				  returnType(returnType),
				  paramTypes(paramTypes) {}
			virtual ~FnTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Fn; }
		};

		class RefTypeNameNode : public TypeNameNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			shared_ptr<TypeNameNode> referencedType;

			inline RefTypeNameNode(const RefTypeNameNode &other) : TypeNameNode(other), referencedType(other.referencedType->duplicate<TypeNameNode>()) {}
			inline RefTypeNameNode(
				shared_ptr<TypeNameNode> referencedType)
				: TypeNameNode(referencedType->getLocation(), referencedType->isConst) {}
			virtual ~RefTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Ref; }
		};

		class BadTypeNameNode : public TypeNameNode {
		private:
			virtual inline shared_ptr<AstNode> doDuplicate() override {
				return make_shared<BadTypeNameNode>(*this);
			}

		public:
			size_t idxStartToken, idxEndToken;

			inline BadTypeNameNode(const BadTypeNameNode &other)
				: TypeNameNode(other.getLocation(), other.isConst) {
			}
			inline BadTypeNameNode(Location loc, size_t idxStartToken, size_t idxEndToken, bool isConst = false)
				: TypeNameNode(loc, isConst), idxStartToken(idxStartToken), idxEndToken(idxEndToken) {}
			virtual ~BadTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Bad; }
		};

		class Compiler;
	}
}

namespace std {
	string to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName = false);
}

#endif
