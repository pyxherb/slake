#ifndef _SLAKE_OBJ_ARRAY_H_
#define _SLAKE_OBJ_ARRAY_H_

#include "object.h"
#include "var.h"
#include <deque>

namespace slake {
	class ArrayObject;

	class ArrayAccessorVarObject : public VarObject {
	public:
		ArrayObject *arrayObject;
		Type elementType;

		SLAKE_API ArrayAccessorVarObject(Runtime *rt, const Type &elementType, ArrayObject *arrayObject);
		SLAKE_API ArrayAccessorVarObject(const ArrayAccessorVarObject &other);
		SLAKE_API virtual ~ArrayAccessorVarObject();

		SLAKE_API virtual Type getVarType(const VarRefContext &context) const override;

		SLAKE_API virtual VarKind getVarKind() const override { return VarKind::ArrayElementAccessor; }
	};

	class ArrayObject : public Object {
	public:
		size_t length = 0;
		Type elementType;
		ArrayAccessorVarObject *accessor;

		SLAKE_API ArrayObject(Runtime *rt, const Type &elementType, ArrayAccessorVarObject *accessor);
		SLAKE_API ArrayObject(const ArrayObject &x);
		SLAKE_API virtual ~ArrayObject();

		SLAKE_API virtual void clear() = 0;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) = 0;
		SLAKE_API virtual void resize(size_t newLength) = 0;

		SLAKE_API virtual ObjectKind getKind() const override;
	};

	class U8ArrayObject;

	class U8ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API U8ArrayAccessorVarObject(Runtime *rt, U8ArrayObject *arrayObject);
		SLAKE_API virtual ~U8ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<U8ArrayAccessorVarObject> alloc(Runtime *rt, U8ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class U8ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		uint8_t *data;
		U8ArrayAccessorVarObject *accessor;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API U8ArrayObject(Runtime *rt, size_t length);
		SLAKE_API U8ArrayObject(const U8ArrayObject &x);
		SLAKE_API virtual ~U8ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<U8ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<U8ArrayObject> alloc(const U8ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class U16ArrayObject;

	class U16ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API U16ArrayAccessorVarObject(Runtime *rt, U16ArrayObject *arrayObject);
		SLAKE_API virtual ~U16ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<U16ArrayAccessorVarObject> alloc(Runtime *rt, U16ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class U16ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		uint16_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API U16ArrayObject(Runtime *rt, size_t length);
		SLAKE_API U16ArrayObject(const U16ArrayObject &x);
		SLAKE_API virtual ~U16ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<U16ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<U16ArrayObject> alloc(const U16ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class U32ArrayObject;

	class U32ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API U32ArrayAccessorVarObject(Runtime *rt, U32ArrayObject *arrayObject);
		SLAKE_API virtual ~U32ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<U32ArrayAccessorVarObject> alloc(Runtime *rt, U32ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class U32ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		uint32_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API U32ArrayObject(Runtime *rt, size_t length);
		SLAKE_API U32ArrayObject(const U32ArrayObject &x);
		SLAKE_API virtual ~U32ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<U32ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<U32ArrayObject> alloc(const U32ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class U64ArrayObject;

	class U64ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API U64ArrayAccessorVarObject(Runtime *rt, U64ArrayObject *arrayObject);
		SLAKE_API virtual ~U64ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<U64ArrayAccessorVarObject> alloc(Runtime *rt, U64ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class U64ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		uint64_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API U64ArrayObject(Runtime *rt, size_t length);
		SLAKE_API U64ArrayObject(const U64ArrayObject &x);
		SLAKE_API virtual ~U64ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<U64ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<U64ArrayObject> alloc(const U64ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class I8ArrayObject;

	class I8ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API I8ArrayAccessorVarObject(Runtime *rt, I8ArrayObject *arrayObject);
		SLAKE_API virtual ~I8ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<I8ArrayAccessorVarObject> alloc(Runtime *rt, I8ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class I8ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		int8_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API I8ArrayObject(Runtime *rt, size_t length);
		SLAKE_API I8ArrayObject(const I8ArrayObject &x);
		SLAKE_API virtual ~I8ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<I8ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<I8ArrayObject> alloc(const I8ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class I16ArrayObject;

	class I16ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API I16ArrayAccessorVarObject(Runtime *rt, I16ArrayObject *arrayObject);
		SLAKE_API virtual ~I16ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<I16ArrayAccessorVarObject> alloc(Runtime *rt, I16ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class I16ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		int16_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API I16ArrayObject(Runtime *rt, size_t length);
		SLAKE_API I16ArrayObject(const I16ArrayObject &x);
		SLAKE_API virtual ~I16ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<I16ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<I16ArrayObject> alloc(const I16ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class I32ArrayObject;

	class I32ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API I32ArrayAccessorVarObject(Runtime *rt, I32ArrayObject *arrayObject);
		SLAKE_API virtual ~I32ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<I32ArrayAccessorVarObject> alloc(Runtime *rt, I32ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class I32ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		int32_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API I32ArrayObject(Runtime *rt, size_t length);
		SLAKE_API I32ArrayObject(const I32ArrayObject &x);
		SLAKE_API virtual ~I32ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<I32ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<I32ArrayObject> alloc(const I32ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class I64ArrayObject;

	class I64ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API I64ArrayAccessorVarObject(Runtime *rt, I64ArrayObject *arrayObject);
		SLAKE_API virtual ~I64ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<I64ArrayAccessorVarObject> alloc(Runtime *rt, I64ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class I64ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		int64_t *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API I64ArrayObject(Runtime *rt, size_t length);
		SLAKE_API I64ArrayObject(const I64ArrayObject &x);
		SLAKE_API virtual ~I64ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<I64ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<I64ArrayObject> alloc(const I64ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class F32ArrayObject;

	class F32ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API F32ArrayAccessorVarObject(Runtime *rt, F32ArrayObject *arrayObject);
		SLAKE_API virtual ~F32ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<F32ArrayAccessorVarObject> alloc(Runtime *rt, F32ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class F32ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		float *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API F32ArrayObject(Runtime *rt, size_t length);
		SLAKE_API F32ArrayObject(const F32ArrayObject &x);
		SLAKE_API virtual ~F32ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<F32ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<F32ArrayObject> alloc(const F32ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class F64ArrayObject;

	class F64ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API F64ArrayAccessorVarObject(Runtime *rt, F64ArrayObject *arrayObject);
		SLAKE_API virtual ~F64ArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<F64ArrayAccessorVarObject> alloc(Runtime *rt, F64ArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class F64ArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		double *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API F64ArrayObject(Runtime *rt, size_t length);
		SLAKE_API F64ArrayObject(const F64ArrayObject &x);
		SLAKE_API virtual ~F64ArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<F64ArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<F64ArrayObject> alloc(const F64ArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class BoolArrayObject;

	class BoolArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API BoolArrayAccessorVarObject(Runtime *rt, BoolArrayObject *arrayObject);
		SLAKE_API virtual ~BoolArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<BoolArrayAccessorVarObject> alloc(Runtime *rt, BoolArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class BoolArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		bool *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API BoolArrayObject(Runtime *rt, size_t length);
		SLAKE_API BoolArrayObject(const BoolArrayObject &x);
		SLAKE_API virtual ~BoolArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<BoolArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<BoolArrayObject> alloc(const BoolArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class ObjectRefArrayObject;

	class ObjectRefArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API ObjectRefArrayAccessorVarObject(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject);
		SLAKE_API virtual ~ObjectRefArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<ObjectRefArrayAccessorVarObject> alloc(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class ObjectRefArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		Object **data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API ObjectRefArrayObject(Runtime *rt, const Type &elementType, size_t length);
		SLAKE_API ObjectRefArrayObject(const ObjectRefArrayObject &x);
		SLAKE_API virtual ~ObjectRefArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<ObjectRefArrayObject> alloc(Runtime *rt, const Type &elementType, size_t length);
		SLAKE_API static HostObjectRef<ObjectRefArrayObject> alloc(const ObjectRefArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class AnyArrayObject;

	class AnyArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		SLAKE_API AnyArrayAccessorVarObject(Runtime *rt, AnyArrayObject *arrayObject);
		SLAKE_API virtual ~AnyArrayAccessorVarObject();

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<AnyArrayAccessorVarObject> alloc(Runtime *rt, AnyArrayObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class AnyArrayObject final : public ArrayObject {
	private:
		SLAKE_API void _resizeUnchecked(size_t newLength);

	public:
		Value *data;

		SLAKE_API virtual void clear() override;
		SLAKE_API virtual [[nodiscard]] InternalExceptionPointer fill(size_t beginIndex, size_t length, const Value &value) override;
		SLAKE_API virtual void resize(size_t newLength) override;

		SLAKE_API AnyArrayObject(Runtime *rt, size_t length);
		SLAKE_API AnyArrayObject(const AnyArrayObject &x);
		SLAKE_API virtual ~AnyArrayObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<AnyArrayObject> alloc(Runtime *rt, size_t length);
		SLAKE_API static HostObjectRef<AnyArrayObject> alloc(const AnyArrayObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	InvalidArrayIndexError *raiseInvalidArrayIndexError(Runtime *rt, size_t index);
}

#endif
