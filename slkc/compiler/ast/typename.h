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
			Char,

			WChar,
			WString,

			Bool,

			Auto,
			Void,

			Any,

			Array,
			Map,
			Fn,

			Custom
		};

		/// @brief Base type name node class.
		/// @note Duplication is unneeded, because type names should not be modified after they created.
		class TypeNameNode : public AstNode {
		private:
			Location _loc;

		public:
			bool isConst;  // For parameters.

			inline TypeNameNode(Location loc, bool isConst = false)
				: _loc(loc), isConst(isConst) {}
			virtual ~TypeNameNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::TypeName; }

			virtual inline Type getTypeId() const = 0;
		};

		template <Type TID>
		class SimpleTypeNameNode : public TypeNameNode {
		public:
			inline SimpleTypeNameNode(Location loc, bool isConst = false)
				: TypeNameNode(loc, isConst) {}
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
		using BoolTypeNameNode = SimpleTypeNameNode<Type::Bool>;
		using AutoTypeNameNode = SimpleTypeNameNode<Type::Auto>;
		using VoidTypeNameNode = SimpleTypeNameNode<Type::Void>;
		using AnyTypeNameNode = SimpleTypeNameNode<Type::Any>;

		class CustomTypeNameNode : public TypeNameNode {
		public:
			Ref ref;
			deque<pair<Ref, shared_ptr<AstNode>>> resolvedPartsOut;

			Compiler *compiler;
			Scope *scope;

			// bool resolved = false;

			inline CustomTypeNameNode(Location loc, Ref ref, Compiler *compiler, Scope *scope, bool isConst = false)
				: TypeNameNode(loc, isConst), ref(ref), compiler(compiler), scope(scope) {}
			virtual ~CustomTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Custom; }
		};

		class ArrayTypeNameNode : public TypeNameNode {
		public:
			shared_ptr<TypeNameNode> elementType;

			inline ArrayTypeNameNode(shared_ptr<TypeNameNode> elementType, bool isConst = false)
				: TypeNameNode(elementType->getLocation(), isConst), elementType(elementType) {}
			virtual ~ArrayTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Array; }
		};

		class MapTypeNameNode : public TypeNameNode {
		public:
			shared_ptr<TypeNameNode> keyType, valueType;

			inline MapTypeNameNode(shared_ptr<TypeNameNode> keyType, shared_ptr<TypeNameNode> valueType, bool isConst = false)
				: TypeNameNode(keyType->getLocation(), isConst), keyType(keyType), valueType(valueType) {}
			virtual ~MapTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Map; }
		};

		class FnTypeNameNode : public TypeNameNode {
		public:
			shared_ptr<TypeNameNode> returnType;
			deque<shared_ptr<TypeNameNode>> paramTypes;

			inline FnTypeNameNode(
				shared_ptr<TypeNameNode> returnType,
				deque<shared_ptr<TypeNameNode>> paramTypes,
				bool isConst = false)
				: TypeNameNode(returnType->getLocation(), isConst),
				  returnType(returnType),
				  paramTypes(paramTypes) {}
			virtual ~FnTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return Type::Fn; }
		};

		class Compiler;
	}
}

namespace std {
	string to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName = false);
}

#endif
