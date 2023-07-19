#ifndef _SLKBCC_TYPENAME_H_
#define _SLKBCC_TYPENAME_H_

#include "base.h"
#include <cstdint>
#include <memory>
#include <deque>

namespace slake {
	namespace bcc {
		using namespace std;

		enum Type : uint8_t {
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
			TYPE_CUSTOM,
			TYPE_GENERIC
		};

		class Ref;

		class TypeName : public ILocated {
		private:
			location _loc;

		public:
			const Type type;

			inline TypeName(location loc, Type type) : _loc(loc), type(type) {}
			virtual ~TypeName() = default;

			virtual location getLocation() const override { return _loc; }
		};

		class CustomTypeName : public TypeName {
		public:
			shared_ptr<Ref> ref;

			inline CustomTypeName(location loc, shared_ptr<Ref> ref) : TypeName(loc, TYPE_CUSTOM), ref(ref) {}
			virtual ~CustomTypeName() = default;
		};

		class ArrayTypeName : public TypeName {
		public:
			shared_ptr<TypeName> elementType;

			inline ArrayTypeName(location loc, shared_ptr<TypeName> elementType)
				: TypeName(loc, TYPE_ARRAY), elementType(elementType) {}
			virtual ~ArrayTypeName() = default;
		};

		class MapTypeName : public TypeName {
		public:
			shared_ptr<TypeName> keyType, type;

			inline MapTypeName(location loc, shared_ptr<TypeName> keyType, shared_ptr<TypeName> type)
				: TypeName(loc, TYPE_MAP), keyType(keyType), type(type) {}
			virtual ~MapTypeName() = default;
		};

		class FnTypeName : public TypeName {
		public:
			shared_ptr<TypeName> returnType;
			deque<shared_ptr<TypeName>> params;

			inline FnTypeName(location loc, shared_ptr<TypeName> returnType, deque<shared_ptr<TypeName>> params = {})
				: TypeName(loc, TYPE_FN), returnType(returnType), params(params) {}
			virtual ~FnTypeName() = default;
		};

		class GenericTypeName : public TypeName {
		public:
			uint8_t index;

			inline GenericTypeName(location loc, uint8_t index) : TypeName(loc, TYPE_GENERIC), index(index) {}
			virtual ~GenericTypeName() = default;
		};
	}
}

#endif
