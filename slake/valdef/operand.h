#ifndef _SLAKE_VALDEF_OPERAND_H_
#define _SLAKE_VALDEF_OPERAND_H_

#include "base.h"
#include "../reg.h"

namespace slake {
	class LocalVarRefValue final : public Value {
	public:
		uint32_t index;

		inline LocalVarRefValue(Runtime *rt, uint32_t index)
			: Value(rt), index(index) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual ~LocalVarRefValue() = default;
		virtual inline Type getType() const override { return TypeId::LVAR_REF; }

		virtual Value *duplicate() const override;

		LocalVarRefValue &operator=(const LocalVarRefValue &x) {
			((Value &)*this) = (Value &)x;
			index = x.index;
			return *this;
		}
		LocalVarRefValue &operator=(LocalVarRefValue &&) = delete;
	};

	class RegRefValue final : public Value {
	public:
		RegId reg;

		inline RegRefValue(Runtime *rt, RegId reg) : Value(rt), reg(reg) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual ~RegRefValue() = default;
		virtual inline Type getType() const override { return TypeId::REG_REF; }

		virtual Value *duplicate() const override;

		RegRefValue &operator=(const RegRefValue &x) {
			((Value &)*this) = (Value &)x;
			reg = x.reg;
			return *this;
		}
		RegRefValue &operator=(RegRefValue &&) = delete;
	};

	class ArgRefValue final : public Value {
	public:
		uint32_t index;

		inline ArgRefValue(Runtime *rt, uint32_t index)
			: Value(rt), index(index) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual ~ArgRefValue() = default;
		virtual inline Type getType() const override { return TypeId::ARG_REF; }

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
