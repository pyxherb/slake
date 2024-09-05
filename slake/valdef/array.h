#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "object.h"
#include "var.h"
#include <deque>

namespace slake {
	class ArrayObject;

	class ArrayAccessorVarObject : public VarObject {
	public:
		ArrayObject *arrayObject;
		Type elementType;

		ArrayAccessorVarObject(Runtime *rt, const Type& elementType, ArrayObject *arrayObject);
		ArrayAccessorVarObject(const ArrayAccessorVarObject &other);
		virtual ~ArrayAccessorVarObject();

		virtual Type getVarType(const VarRefContext &context) const override;

		virtual VarKind getVarKind() const override { return VarKind::ArrayElementAccessor; }
	};

	class ArrayObject : public Object {
	public:
		size_t length = 0;
		Type elementType;
		ArrayAccessorVarObject *accessor;

		ArrayObject(Runtime *rt, const Type &elementType, ArrayAccessorVarObject *accessor);
		ArrayObject(const ArrayObject &x);
		virtual ~ArrayObject();

		virtual void clear() = 0;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) = 0;
		virtual void resize(size_t newLength) = 0;

		virtual ObjectKind getKind() const override;
	};

	class U8ArrayObject;

	class U8ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		U8ArrayAccessorVarObject(Runtime *rt, U8ArrayObject *arrayObject);
		virtual ~U8ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<U8ArrayAccessorVarObject> alloc(Runtime *rt, U8ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class U8ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		uint8_t *data;
		U8ArrayAccessorVarObject *accessor;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		U8ArrayObject(Runtime *rt, size_t length);
		U8ArrayObject(const U8ArrayObject &x);
		virtual ~U8ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<U8ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<U8ArrayObject> alloc(const U8ArrayObject *other);
		virtual void dealloc() override;
	};

	class U16ArrayObject;

	class U16ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		U16ArrayAccessorVarObject(Runtime *rt, U16ArrayObject *arrayObject);
		virtual ~U16ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<U16ArrayAccessorVarObject> alloc(Runtime *rt, U16ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class U16ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		uint16_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		U16ArrayObject(Runtime *rt, size_t length);
		U16ArrayObject(const U16ArrayObject &x);
		virtual ~U16ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<U16ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<U16ArrayObject> alloc(const U16ArrayObject *other);
		virtual void dealloc() override;
	};

	class U32ArrayObject;

	class U32ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		U32ArrayAccessorVarObject(Runtime *rt, U32ArrayObject *arrayObject);
		virtual ~U32ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<U32ArrayAccessorVarObject> alloc(Runtime *rt, U32ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class U32ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		uint32_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		U32ArrayObject(Runtime *rt, size_t length);
		U32ArrayObject(const U32ArrayObject &x);
		virtual ~U32ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<U32ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<U32ArrayObject> alloc(const U32ArrayObject *other);
		virtual void dealloc() override;
	};

	class U64ArrayObject;

	class U64ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		U64ArrayAccessorVarObject(Runtime *rt, U64ArrayObject *arrayObject);
		virtual ~U64ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<U64ArrayAccessorVarObject> alloc(Runtime *rt, U64ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class U64ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		uint64_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		U64ArrayObject(Runtime *rt, size_t length);
		U64ArrayObject(const U64ArrayObject &x);
		virtual ~U64ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<U64ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<U64ArrayObject> alloc(const U64ArrayObject *other);
		virtual void dealloc() override;
	};

	class I8ArrayObject;

	class I8ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		I8ArrayAccessorVarObject(Runtime *rt, I8ArrayObject *arrayObject);
		virtual ~I8ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<I8ArrayAccessorVarObject> alloc(Runtime *rt, I8ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class I8ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		int8_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		I8ArrayObject(Runtime *rt, size_t length);
		I8ArrayObject(const I8ArrayObject &x);
		virtual ~I8ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<I8ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<I8ArrayObject> alloc(const I8ArrayObject *other);
		virtual void dealloc() override;
	};

	class I16ArrayObject;

	class I16ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		I16ArrayAccessorVarObject(Runtime *rt, I16ArrayObject *arrayObject);
		virtual ~I16ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<I16ArrayAccessorVarObject> alloc(Runtime *rt, I16ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class I16ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		int16_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		I16ArrayObject(Runtime *rt, size_t length);
		I16ArrayObject(const I16ArrayObject &x);
		virtual ~I16ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<I16ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<I16ArrayObject> alloc(const I16ArrayObject *other);
		virtual void dealloc() override;
	};

	class I32ArrayObject;

	class I32ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		I32ArrayAccessorVarObject(Runtime *rt, I32ArrayObject *arrayObject);
		virtual ~I32ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<I32ArrayAccessorVarObject> alloc(Runtime *rt, I32ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class I32ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		int32_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		I32ArrayObject(Runtime *rt, size_t length);
		I32ArrayObject(const I32ArrayObject &x);
		virtual ~I32ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<I32ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<I32ArrayObject> alloc(const I32ArrayObject *other);
		virtual void dealloc() override;
	};

	class I64ArrayObject;

	class I64ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		I64ArrayAccessorVarObject(Runtime *rt, I64ArrayObject *arrayObject);
		virtual ~I64ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<I64ArrayAccessorVarObject> alloc(Runtime *rt, I64ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class I64ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		int64_t *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		I64ArrayObject(Runtime *rt, size_t length);
		I64ArrayObject(const I64ArrayObject &x);
		virtual ~I64ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<I64ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<I64ArrayObject> alloc(const I64ArrayObject *other);
		virtual void dealloc() override;
	};

	class F32ArrayObject;

	class F32ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		F32ArrayAccessorVarObject(Runtime *rt, F32ArrayObject *arrayObject);
		virtual ~F32ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<F32ArrayAccessorVarObject> alloc(Runtime *rt, F32ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class F32ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		float *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		F32ArrayObject(Runtime *rt, size_t length);
		F32ArrayObject(const F32ArrayObject &x);
		virtual ~F32ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<F32ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<F32ArrayObject> alloc(const F32ArrayObject *other);
		virtual void dealloc() override;
	};

	class F64ArrayObject;

	class F64ArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		F64ArrayAccessorVarObject(Runtime *rt, F64ArrayObject *arrayObject);
		virtual ~F64ArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<F64ArrayAccessorVarObject> alloc(Runtime *rt, F64ArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class F64ArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		double *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		F64ArrayObject(Runtime *rt, size_t length);
		F64ArrayObject(const F64ArrayObject &x);
		virtual ~F64ArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<F64ArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<F64ArrayObject> alloc(const F64ArrayObject *other);
		virtual void dealloc() override;
	};

	class BoolArrayObject;

	class BoolArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		BoolArrayAccessorVarObject(Runtime *rt, BoolArrayObject *arrayObject);
		virtual ~BoolArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<BoolArrayAccessorVarObject> alloc(Runtime *rt, BoolArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class BoolArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		bool *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		BoolArrayObject(Runtime *rt, size_t length);
		BoolArrayObject(const BoolArrayObject &x);
		virtual ~BoolArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<BoolArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<BoolArrayObject> alloc(const BoolArrayObject *other);
		virtual void dealloc() override;
	};

	class ObjectRefArrayObject;

	class ObjectRefArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		ObjectRefArrayAccessorVarObject(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject);
		virtual ~ObjectRefArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<ObjectRefArrayAccessorVarObject> alloc(Runtime *rt, const Type &elementType, ObjectRefArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class ObjectRefArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		Object **data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		ObjectRefArrayObject(Runtime *rt, const Type &elementType, size_t length);
		ObjectRefArrayObject(const ObjectRefArrayObject &x);
		virtual ~ObjectRefArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<ObjectRefArrayObject> alloc(Runtime *rt, const Type &elementType, size_t length);
		static HostObjectRef<ObjectRefArrayObject> alloc(const ObjectRefArrayObject *other);
		virtual void dealloc() override;
	};

	class AnyArrayObject;

	class AnyArrayAccessorVarObject : public ArrayAccessorVarObject {
	public:
		AnyArrayAccessorVarObject(Runtime *rt, AnyArrayObject *arrayObject);
		virtual ~AnyArrayAccessorVarObject();

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<AnyArrayAccessorVarObject> alloc(Runtime *rt, AnyArrayObject *arrayObject);
		virtual void dealloc() override;
	};

	class AnyArrayObject final : public ArrayObject {
	private:
		void _resizeUnchecked(size_t newLength);

	public:
		Value *data;

		virtual void clear() override;
		virtual void fill(size_t beginIndex, size_t length, const Value &value) override;
		virtual void resize(size_t newLength) override;

		AnyArrayObject(Runtime *rt, size_t length);
		AnyArrayObject(const AnyArrayObject &x);
		virtual ~AnyArrayObject();

		Object *duplicate() const override;

		static HostObjectRef<AnyArrayObject> alloc(Runtime *rt, size_t length);
		static HostObjectRef<AnyArrayObject> alloc(const AnyArrayObject *other);
		virtual void dealloc() override;
	};
}

#endif
