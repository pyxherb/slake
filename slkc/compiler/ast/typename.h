#ifndef _SLKC_COMPILER_TYPENAME_H_
#define _SLKC_COMPILER_TYPENAME_H_

#include <cstdint>

#include "ref.h"

namespace slake {
	namespace slkc {
		enum TypeName : uint8_t {
			TYPE_I8,
			TYPE_I16,
			TYPE_I32,
			TYPE_I64,
			TYPE_ISIZE,
			TYPE_U8,
			TYPE_U16,
			TYPE_U32,
			TYPE_U64,
			TYPE_USIZE,
			TYPE_F32,
			TYPE_F64,
			TYPE_STRING,
			TYPE_BOOL,
			TYPE_AUTO,
			TYPE_VOID,
			TYPE_ANY,
			TYPE_ARRAY,
			TYPE_MAP,
			TYPE_FN,
			TYPE_CUSTOM
		};

		class TypeName {
		public:
			const TypeName type;

			inline TypeName(TypeName type) : type(type) {}
			virtual ~TypeName() = default;
		};

		class CustomTypeName : public TypeName {
		public:
			shared_ptr<Ref> ref;

			inline CustomTypeName(shared_ptr<Ref> ref) : TypeName(TYPE_CUSTOM), ref(ref) {}
			virtual ~CustomTypeName() = default;
		};

		class ArrayTypeName : public TypeName {
		public:
			shared_ptr<TypeName> elementType;

			inline ArrayTypeName(shared_ptr<TypeName> elementType)
				: TypeName(TYPE_ARRAY), elementType(elementType) {}
			virtual ~ArrayTypeName() = default;
		};

		class MapTypeName : public TypeName {
		public:
			shared_ptr<TypeName> keyType, type;

			inline MapTypeName(shared_ptr<TypeName> keyType, shared_ptr<TypeName> type)
				: TypeName(TYPE_MAP), keyType(keyType), type(type) {}
			virtual ~MapTypeName() = default;
		};

		class FnTypeName : public TypeName {
		public:
			shared_ptr<TypeName> elementType;

			inline FnTypeName(shared_ptr<TypeName> elementType)
				: TypeName(TYPE_FN), elementType(elementType) {}
			virtual ~FnTypeName() = default;
		};
	}
}

#endif