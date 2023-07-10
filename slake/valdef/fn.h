#ifndef _SLAKE_VALDEF_FN_H_
#define _SLAKE_VALDEF_FN_H_

#include <slake/opcode.h>

#include <functional>
#include <deque>

#include "member.h"

namespace slake {
	struct Instruction final {
		Opcode opcode = (Opcode)0xff;
		ValueRef<> operands[3] = {};
		uint8_t nOperands = 0;

		inline uint8_t getOperandCount() {
			return nOperands;
		}
		inline ~Instruction() {
		}
	};

	class BasicFnValue : public MemberValue {
	protected:
		std::vector<Type> _paramTypes;
		Type _returnType;

		friend class Runtime;
		friend class ClassValue;

	public:
		inline BasicFnValue(
			Runtime *rt,
			AccessModifier access,
			Type returnType)
			: MemberValue(rt, access) {
		}
		virtual ~BasicFnValue() = default;

		virtual Type getType() const override;
		virtual Type getReturnType() const;

		inline const decltype(_paramTypes) getParamTypes() const {
			return _paramTypes;
		}

		virtual bool isAbstract() const = 0;

		BasicFnValue &operator=(const BasicFnValue &) = delete;
		BasicFnValue &operator=(const BasicFnValue &&) = delete;
	};

	class FnValue : public BasicFnValue {
	protected:
		Instruction *_body = nullptr;
		const uint32_t _nIns;

		friend class Runtime;

		friend class ObjectValue;

		friend struct FnComparator;

	public:
		inline FnValue(Runtime *rt, uint32_t nIns, AccessModifier access, Type returnType)
			: _nIns(nIns),
			  BasicFnValue(rt, access, returnType) {
			if (nIns)
				_body = new Instruction[nIns];
			reportSizeToRuntime(sizeof(*this) + sizeof(Instruction) * nIns);
		}
		virtual ~FnValue();

		inline uint32_t getInsCount() const noexcept { return _nIns; }
		inline const Instruction *getBody() const noexcept { return _body; }
		inline Instruction *getBody() noexcept { return _body; }

		virtual ValueRef<> call(uint8_t nArgs, ValueRef<> *args) const override;

		virtual bool isAbstract() const override {
			return _nIns == 0;
		}

		FnValue &operator=(const FnValue &) = delete;
		FnValue &operator=(const FnValue &&) = delete;
	};

	using NativeFnCallback = std::function<ValueRef<>(Runtime *rt, uint8_t nArgs, ValueRef<> *args)>;
	class NativeFnValue final : public BasicFnValue {
	protected:
		NativeFnCallback _body;
		friend class ClassValue;

	public:
		inline NativeFnValue(Runtime *rt, NativeFnCallback body, AccessModifier access, Type returnType, std::string name = "", Value *parent = nullptr)
			: BasicFnValue(rt, access | ACCESS_NATIVE, returnType), _body(body) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual ~NativeFnValue() = default;

		inline const NativeFnCallback getBody() const noexcept { return _body; }

		virtual ValueRef<> call(uint8_t nArgs, ValueRef<> *args) const override;

		virtual bool isAbstract() const override { return (bool)_body; }

		NativeFnValue &operator=(const NativeFnValue &) = delete;
		NativeFnValue &operator=(const NativeFnValue &&) = delete;
	};
}

#endif
