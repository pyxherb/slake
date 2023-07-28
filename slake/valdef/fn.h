#ifndef _SLAKE_VALDEF_FN_H_
#define _SLAKE_VALDEF_FN_H_

#include <slake/opcode.h>

#include <functional>
#include <deque>

#include "member.h"
#include "generic.h"

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
		GenericParamList genericParams;
		std::deque<Type> paramTypes;
		Type returnType;

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

		inline const GenericParamList getGenericParams() const {
			return genericParams;
		}

		inline const std::deque<Type> getParamTypes() const {
			return paramTypes;
		}

		virtual bool isAbstract() const = 0;

		inline BasicFnValue &operator=(const BasicFnValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;

			genericParams = x.genericParams;
			paramTypes = x.paramTypes;
			returnType = x.returnType;

			return *this;
		}
		BasicFnValue &operator=(const BasicFnValue &&) = delete;
	};

	class FnValue : public BasicFnValue {
	protected:
		Instruction *body = nullptr;
		uint32_t nIns;

		friend class Runtime;

		friend class ObjectValue;

		friend struct FnComparator;

	public:
		inline FnValue(Runtime *rt, uint32_t nIns, AccessModifier access, Type returnType)
			: nIns(nIns),
			  BasicFnValue(rt, access, returnType) {
			if (nIns)
				body = new Instruction[nIns];
			reportSizeToRuntime(sizeof(*this) + sizeof(Instruction) * nIns);
		}
		virtual ~FnValue();

		inline uint32_t getInsCount() const noexcept { return nIns; }
		inline const Instruction *getBody() const noexcept { return body; }
		inline Instruction *getBody() noexcept { return body; }

		virtual ValueRef<> call(std::deque<ValueRef<>> args) const override;

		virtual bool isAbstract() const override {
			return nIns == 0;
		}

		Value *duplicate() const override;

		inline FnValue &operator=(const FnValue &x) {
			((BasicFnValue &)*this) = (BasicFnValue &)x;

			// Delete existing function body.
			if (body) {
				delete[] body;
				body = nullptr;
			}

			// Copy the function body if the source function is not abstract.
			if (x.body) {
				body = new Instruction[x.nIns];

				// Copy each instruction.
				for (size_t i = 0; i < x.nIns; ++i) {
					// Duplicate current instruction from the source function.
					auto ins = x.body[i];

					// Copy each operand.
					for (size_t j = 0; j < ins.nOperands; ++j) {
						auto &operand = ins.operands[j];
						if (operand)
							operand = operand->duplicate();
					}

					// Move current instruction into the function body.
					body[i] = ins;
				}
			}
			nIns = x.nIns;

			return *this;
		}
		FnValue &operator=(const FnValue &&) = delete;
	};

	using NativeFnCallback = std::function<ValueRef<>(Runtime *rt, std::deque<ValueRef<>> args)>;
	class NativeFnValue final : public BasicFnValue {
	protected:
		NativeFnCallback body;
		friend class ClassValue;

	public:
		inline NativeFnValue(Runtime *rt, NativeFnCallback body, AccessModifier access, Type returnType)
			: BasicFnValue(rt, access | ACCESS_NATIVE, returnType), body(body) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual ~NativeFnValue() = default;

		inline const NativeFnCallback getBody() const noexcept { return body; }

		virtual ValueRef<> call(std::deque<ValueRef<>> args) const override;

		virtual bool isAbstract() const override { return (bool)body; }

		Value *duplicate() const override;

		inline NativeFnValue &operator=(const NativeFnValue &x) {
			((BasicFnValue &)*this) = (BasicFnValue &)x;
			body = x.body;
			return *this;
		}
		NativeFnValue &operator=(const NativeFnValue &&) = delete;
	};
}

#endif
