#ifndef _SLAKE_VALDEF_FN_H_
#define _SLAKE_VALDEF_FN_H_

#include "member.h"
#include <slake/opcode.h>
#include <vector>
#include <functional>

namespace Slake {
	struct Instruction final {
		Opcode opcode = (Opcode)0xff;
		ValueRef<> operands[3] = { nullptr };
		uint8_t nOperands = 0;

		inline uint8_t getOperandCount() {
			return nOperands;
		}
		inline ~Instruction() {
		}
	};

	class BasicFnValue : public MemberValue {
	protected:
		std::vector<Type> _params;
		Type _returnType;

		friend class Runtime;
		friend class ClassValue;

	public:
		inline BasicFnValue(Runtime *rt, AccessModifier access, Type returnType, Value *parent) : MemberValue(rt, access, parent) {
		}
		virtual inline ~BasicFnValue() {}

		virtual inline Type getType() const override { return ValueType::FN; }

		BasicFnValue &operator=(const BasicFnValue &) = delete;
		BasicFnValue &operator=(const BasicFnValue &&) = delete;
	};

	class FnValue final : public BasicFnValue {
	protected:
		Instruction *const _body;
		const uint32_t _nIns;

		friend class Runtime;
		friend class ClassValue;

	public:
		inline FnValue(Runtime *rt, uint32_t nIns, AccessModifier access, Type returnType, Value *parent)
			: _nIns(nIns),
			  _body(new Instruction[nIns]),
			  BasicFnValue(rt, access, returnType, parent) {
			if (!nIns)
				throw std::invalid_argument("Invalid instruction count");
			reportSizeToRuntime(sizeof(*this) + sizeof(Instruction) * nIns);
		}
		virtual inline ~FnValue() { delete[] _body; }

		inline uint32_t getInsCount() const noexcept { return _nIns; }
		inline const Instruction *getBody() const noexcept { return _body; }
		inline Instruction *getBody() noexcept { return _body; }

		virtual ValueRef<> call(uint8_t nArgs, ValueRef<> *args) override;

		virtual std::string toString() const override;

		FnValue &operator=(const FnValue &) = delete;
		FnValue &operator=(const FnValue &&) = delete;
	};

	using NativeFnCallback = std::function<ValueRef<>(Runtime *rt, uint8_t nArgs, ValueRef<> *args)>;
	class NativeFnValue final : public BasicFnValue {
	protected:
		NativeFnCallback _body;
		friend class ClassValue;

	public:
		inline NativeFnValue(Runtime *rt, NativeFnCallback body, AccessModifier access, Type returnType, Value *parent = nullptr)
			: BasicFnValue(rt, access | ACCESS_NATIVE, returnType, parent), _body(body) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~NativeFnValue() {}

		inline const NativeFnCallback getBody() const noexcept { return _body; }

		virtual ValueRef<> call(uint8_t nArgs, ValueRef<> *args) override { return _body(getRuntime(), nArgs, args); }

		virtual inline std::string toString() const override {
			std::string s = Value::toString() + ",\"callbackType\":\"" + _body.target_type().name() + "\"";
			return s;
		}

		NativeFnValue &operator=(const NativeFnValue &) = delete;
		NativeFnValue &operator=(const NativeFnValue &&) = delete;
	};
}

namespace std {
	inline std::string to_string(const Slake::Instruction &&ins) {
		std::string s = "{\"opcode\":" + std::to_string((uint8_t)ins.opcode) + ",\"operands\":[";
		for (size_t i = 0; i < ins.nOperands; i++) {
			s += (i ? "," : "") + ins.operands[i];
		}
		s += "]}";
		return s;
	}
	inline std::string to_string(const Slake::Instruction &ins) {
		return to_string(move(ins));
	}
}

#endif
