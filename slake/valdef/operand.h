#ifndef _SLAKE_VALDEF_OPERAND_H_
#define _SLAKE_VALDEF_OPERAND_H_

#include "base.h"

namespace slake {
	class LocalVarRefValue final : public Value {
	public:
		int32_t index;
		bool unwrapValue;

		LocalVarRefValue(Runtime *rt, int32_t index, bool unwrapValue = false);
		virtual ~LocalVarRefValue();

		virtual inline Type getType() const override { return TypeId::LocalVarRef; }

		virtual Value *duplicate() const override;

		inline LocalVarRefValue &operator=(const LocalVarRefValue &x) {
			((Value &)*this) = (Value &)x;
			index = x.index;
			unwrapValue = x.unwrapValue;
			return *this;
		}
		LocalVarRefValue &operator=(LocalVarRefValue &&) = delete;
	};

	class RegRefValue final : public Value {
	public:
		int32_t index;
		bool unwrapValue;

		inline RegRefValue(Runtime *rt, int32_t index, bool unwrapValue = false)
			: Value(rt), index(index), unwrapValue(unwrapValue) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual ~RegRefValue();
		virtual inline Type getType() const override { return TypeId::RegRef; }

		virtual Value *duplicate() const override;

		inline RegRefValue &operator=(const RegRefValue &x) {
			((Value &)*this) = (Value &)x;
			index = x.index;
			unwrapValue = x.unwrapValue;
			return *this;
		}
		RegRefValue &operator=(RegRefValue &&) = delete;
	};

	class ArgRefValue final : public Value {
	public:
		uint32_t index;
		bool unwrapValue;

		inline ArgRefValue(Runtime *rt, uint32_t index, bool unwrapValue = false)
			: Value(rt), index(index), unwrapValue(unwrapValue) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual ~ArgRefValue();
		virtual inline Type getType() const override { return TypeId::ArgRef; }

		virtual Value *duplicate() const override;

		ArgRefValue &operator=(const ArgRefValue &x) {
			((Value &)*this) = (Value &)x;
			index = x.index;
			return *this;
		}
		ArgRefValue &operator=(ArgRefValue &&) = delete;
	};
}

#endif
