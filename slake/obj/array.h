#ifndef _SLAKE_OBJ_ARRAY_H_
#define _SLAKE_OBJ_ARRAY_H_

#include "object.h"
#include "var.h"

namespace slake {
	class ArrayObject;

	class ArrayObject : public Object {
	public:
		size_t length = 0;
		TypeRef element_type;
		size_t element_size;
		size_t element_alignment;
		void *data = nullptr;

		SLAKE_API ArrayObject(Runtime *rt, peff::Alloc *self_allocator, const TypeRef &element_type, size_t element_size);
		SLAKE_API virtual ~ArrayObject();

		SLAKE_API static ArrayObject *alloc(Runtime *rt, const TypeRef &element_type, size_t element_size);

		SLAKE_API virtual void dealloc() override;
	};

	InternalExceptionPointer raise_invalid_array_index_error(Runtime *rt, size_t index);
}

#endif
