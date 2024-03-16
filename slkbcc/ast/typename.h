#ifndef _SLKBCC_TYPENAME_H_
#define _SLKBCC_TYPENAME_H_

#include "base.h"
#include <cstdint>
#include <memory>
#include <deque>

namespace slake {
	namespace bcc {
		using namespace std;

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
			Bool,
			Auto,
			Void,
			Any,
			Array,
			Map,
			Fn,
			Custom,
			Generic
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

			inline CustomTypeName(location loc, shared_ptr<Ref> ref) : TypeName(loc, Type::Custom), ref(ref) {}
			virtual ~CustomTypeName() = default;
		};

		class ArrayTypeName : public TypeName {
		public:
			shared_ptr<TypeName> elementType;

			inline ArrayTypeName(location loc, shared_ptr<TypeName> elementType)
				: TypeName(loc, Type::Array), elementType(elementType) {}
			virtual ~ArrayTypeName() = default;
		};

		class MapTypeName : public TypeName {
		public:
			shared_ptr<TypeName> keyType, type;

			inline MapTypeName(location loc, shared_ptr<TypeName> keyType, shared_ptr<TypeName> type)
				: TypeName(loc, Type::Map), keyType(keyType), type(type) {}
			virtual ~MapTypeName() = default;
		};

		class FnTypeName : public TypeName {
		public:
			shared_ptr<TypeName> returnType;
			deque<shared_ptr<TypeName>> params;

			inline FnTypeName(location loc, shared_ptr<TypeName> returnType, deque<shared_ptr<TypeName>> params = {})
				: TypeName(loc, Type::Fn), returnType(returnType), params(params) {}
			virtual ~FnTypeName() = default;
		};

		class GenericTypeName : public TypeName {
		public:
			uint8_t index;

			inline GenericTypeName(location loc, uint8_t index) : TypeName(loc, Type::Generic), index(index) {}
			virtual ~GenericTypeName() = default;
		};
	}
}

#endif
