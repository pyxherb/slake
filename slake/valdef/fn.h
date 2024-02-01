#ifndef _SLAKE_VALDEF_FN_H_
#define _SLAKE_VALDEF_FN_H_

#include <slake/opcode.h>
#include <slake/slxfmt.h>

#include <functional>
#include <deque>
#include <memory>

#include "member.h"
#include "generic.h"

namespace slake {
	struct Context;

	struct Instruction final {
		Opcode opcode = (Opcode)0xffff;
		std::deque<ValueRef<>> operands;
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
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
		}
		virtual ~BasicFnValue();

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
		BasicFnValue &operator=(BasicFnValue &&) = delete;
	};

	class FnValue : public BasicFnValue {
	protected:
		Instruction *body = nullptr;
		uint32_t nIns;

		friend class Runtime;
		friend class ObjectValue;
		friend struct FnComparator;

	public:
		std::deque<slxfmt::SourceLocDesc> sourceLocDescs;

		slxfmt::SourceLocDesc *getSourceLocationInfo(uint32_t offIns) {
			for (auto &i : sourceLocDescs) {
				if ((offIns >= i.offIns) &&
					(offIns < i.offIns + i.nIns)) {
					return &i;
				}
			}

			return nullptr;
		}

		const slxfmt::SourceLocDesc *getSourceLocationInfo(uint32_t offIns) const {
			return ((FnValue*)this)->getSourceLocationInfo(offIns);
		}

#if SLAKE_ENABLE_DEBUGGER
		std::set<uint32_t> breakpoints;
#endif

		FnValue(Runtime *rt, uint32_t nIns, AccessModifier access, Type returnType);
		virtual ~FnValue();

		inline uint32_t getInsCount() const noexcept { return nIns; }
		inline const Instruction *getBody() const noexcept { return body; }
		inline Instruction *getBody() noexcept { return body; }

		ValueRef<> exec(std::shared_ptr<Context> context) const;
		virtual ValueRef<> call(std::deque<ValueRef<>> args) const override;

		virtual bool isAbstract() const override {
			return nIns == 0;
		}

		Value *duplicate() const override;

		FnValue &operator=(const FnValue &x);
		FnValue &operator=(FnValue &&) = delete;
	};

	using NativeFnCallback = std::function<ValueRef<>(Runtime *rt, std::deque<ValueRef<>> args)>;
	class NativeFnValue final : public BasicFnValue {
	protected:
		NativeFnCallback body;
		friend class ClassValue;

	public:
		NativeFnValue(Runtime *rt, NativeFnCallback body, AccessModifier access, Type returnType);
		virtual ~NativeFnValue();

		inline const NativeFnCallback getBody() const noexcept { return body; }

		virtual ValueRef<> call(std::deque<ValueRef<>> args) const override;

		virtual bool isAbstract() const override { return (bool)body; }

		Value *duplicate() const override;

		inline NativeFnValue &operator=(const NativeFnValue &x) {
			((BasicFnValue &)*this) = (BasicFnValue &)x;
			body = x.body;
			return *this;
		}
		NativeFnValue &operator=(NativeFnValue &&) = delete;
	};
}

#endif
