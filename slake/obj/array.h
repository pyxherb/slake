#ifndef _SLAKE_OBJ_ARRAY_H_
#define _SLAKE_OBJ_ARRAY_H_

#include "object.h"
#include "var.h"

namespace slake {
	class ArrayObject;

	class ArrayObject : public Object {
	public:
		size_t length = 0;
		Type elementType;
		size_t elementSize;
		void *data = nullptr;

		SLAKE_API ArrayObject(Runtime *rt, const Type &elementType, size_t elementSize);
		SLAKE_API virtual ~ArrayObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API static ArrayObject *alloc(Runtime *rt, const Type &elementType, size_t elementSize);

		SLAKE_API virtual void dealloc() override;
	};

	InvalidArrayIndexError *raiseInvalidArrayIndexError(Runtime *rt, size_t index);
}

#endif
