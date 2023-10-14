#ifndef _SLKC_COMPILER_AST_TYPENAME_H_
#define _SLKC_COMPILER_AST_TYPENAME_H_

#include <cstdint>
#include <variant>

#include "ref.h"

namespace slake {
	namespace slkc {
		enum Type : uint8_t {
			TYPE_I8,
			TYPE_I16,
			TYPE_I32,
			TYPE_I64,

			TYPE_U8,
			TYPE_U16,
			TYPE_U32,
			TYPE_U64,

			TYPE_F32,
			TYPE_F64,

			TYPE_STRING,
			TYPE_CHAR,

			TYPE_WCHAR,
			TYPE_WSTRING,

			TYPE_BOOL,

			TYPE_AUTO,
			TYPE_VOID,

			TYPE_ANY,

			TYPE_ARRAY,
			TYPE_MAP,
			TYPE_FN,

			TYPE_CUSTOM
		};

		class TypeNameNode : public AstNode {
		private:
			Location _loc;

		public:
			bool isConst; // For parameters.
			bool isRValue;

			inline TypeNameNode(Location loc, bool isConst = false)
				: _loc(loc), isConst(isConst) {}
			virtual ~TypeNameNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_TYPENAME; }

			virtual inline Type getTypeId() const = 0;
		};

		template<Type TID>
		class SimpleTypeNameNode : public TypeNameNode {
		public:
			inline SimpleTypeNameNode(Location loc, bool isConst = false)
				: TypeNameNode(loc, isConst) {}
			virtual ~SimpleTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return TID; }
		};

		using I8TypeNameNode = SimpleTypeNameNode<TYPE_I8>;
		using I16TypeNameNode = SimpleTypeNameNode<TYPE_I16>;
		using I32TypeNameNode = SimpleTypeNameNode<TYPE_I32>;
		using I64TypeNameNode = SimpleTypeNameNode<TYPE_I64>;
		using U8TypeNameNode = SimpleTypeNameNode<TYPE_U8>;
		using U16TypeNameNode = SimpleTypeNameNode<TYPE_U16>;
		using U32TypeNameNode = SimpleTypeNameNode<TYPE_U32>;
		using U64TypeNameNode = SimpleTypeNameNode<TYPE_U64>;
		using F32TypeNameNode = SimpleTypeNameNode<TYPE_F32>;
		using F64TypeNameNode = SimpleTypeNameNode<TYPE_F64>;
		using StringTypeNameNode = SimpleTypeNameNode<TYPE_STRING>;
		using BoolTypeNameNode = SimpleTypeNameNode<TYPE_BOOL>;
		using AutoTypeNameNode = SimpleTypeNameNode<TYPE_AUTO>;
		using VoidTypeNameNode = SimpleTypeNameNode<TYPE_VOID>;
		using AnyTypeNameNode = SimpleTypeNameNode<TYPE_ANY>;

		class CustomTypeNameNode : public TypeNameNode {
		public:
			Ref ref;
			shared_ptr<AstNode> resolvedDest;
			bool resolved = false;

			inline CustomTypeNameNode(Location loc, Ref ref, bool isConst = false)
				: TypeNameNode(loc, isConst), ref(ref) {}
			virtual ~CustomTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return TYPE_CUSTOM; }
		};

		class ArrayTypeNameNode : public TypeNameNode {
		public:
			shared_ptr<TypeNameNode> elementType;

			inline ArrayTypeNameNode(shared_ptr<TypeNameNode> elementType, bool isConst = false)
				: TypeNameNode(elementType->getLocation(), isConst), elementType(elementType) {}
			virtual ~ArrayTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return TYPE_ARRAY; }
		};

		class MapTypeNameNode : public TypeNameNode {
		public:
			shared_ptr<TypeNameNode> keyType, valueType;

			inline MapTypeNameNode(shared_ptr<TypeNameNode> keyType, shared_ptr<TypeNameNode> valueType, bool isConst = false)
				: TypeNameNode(keyType->getLocation(), isConst), keyType(keyType), valueType(valueType) {}
			virtual ~MapTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return TYPE_MAP; }
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
				returnType(returnType), paramTypes(paramTypes) {}
			virtual ~FnTypeNameNode() = default;

			virtual inline Type getTypeId() const override { return TYPE_FN; }
		};
	}
}

namespace std {
	string to_string(shared_ptr<slake::slkc::TypeNameNode> typeName);
}

#endif
