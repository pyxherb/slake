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
		std::deque<Value *> operands;
	};

	enum class FnOverloadingKind {
		Regular = 0,
		Native
	};

	using OverloadingFlags = uint32_t;

	constexpr static OverloadingFlags
		OL_VARG = 0x01;

	class FnValue;

	class FnOverloadingValue : public Value {
	public:
		FnValue *fnValue;

		AccessModifier access;

		GenericParamList genericParams;
		std::unordered_map<std::string, Type> mappedGenericArgs;

		std::deque<Type> paramTypes;
		Type returnType;

		OverloadingFlags overloadingFlags = 0;

		FnOverloadingValue(
			FnValue *fnValue,
			AccessModifier access,
			std::deque<Type> paramTypes,
			Type returnType);
		virtual ~FnOverloadingValue();

		virtual inline Type getType() const { return TypeId::FnOverloading; }

		virtual FnOverloadingKind getOverloadingKind() const = 0;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const = 0;

		virtual FnOverloadingValue *duplicate() const = 0;

		inline FnOverloadingValue &operator=(const FnOverloadingValue &other) {
			fnValue = other.fnValue;

			access = other.access;

			genericParams = other.genericParams;
			mappedGenericArgs = other.mappedGenericArgs;

			paramTypes = other.paramTypes;
			returnType = other.returnType;

			overloadingFlags = other.overloadingFlags;

			return *this;
		}
	};

	class RegularFnOverloadingValue : public FnOverloadingValue {
	public:
		std::deque<slxfmt::SourceLocDesc> sourceLocDescs;
		std::deque<Instruction> instructions;

		inline RegularFnOverloadingValue(
			FnValue *fnValue,
			AccessModifier access,
			std::deque<Type> paramTypes,
			Type returnType)
			: FnOverloadingValue(
				  fnValue,
				  access,
				  paramTypes,
				  returnType) {}
		virtual ~RegularFnOverloadingValue() = default;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const override;

		virtual FnOverloadingValue *duplicate() const override;

		inline RegularFnOverloadingValue &operator=(const RegularFnOverloadingValue &other) {
			*(FnOverloadingValue *)this = (const FnOverloadingValue &)other;

			sourceLocDescs = other.sourceLocDescs;

			instructions.resize(other.instructions.size());
			for (size_t i = 0; i < instructions.size(); ++i) {
				instructions[i].opcode = other.instructions[i].opcode;

				// Duplicate each of the operands.
				instructions[i].operands.resize(other.instructions[i].operands.size());
				for (size_t j = 0; j < other.instructions[i].operands.size(); ++j) {
					instructions[i].operands[j] =
						other.instructions[i].operands[j]
							? other.instructions[i].operands[j]->duplicate()
							: nullptr;
				}
			}

			return *this;
		}
	};

	using NativeFnCallback =
		std::function<ValueRef<>(
			Runtime *rt,
			Value *thisObject,
			std::deque<Value *> args,
			const std::unordered_map<std::string, Type> &mappedGenericArgs)>;

	class NativeFnOverloadingValue : public FnOverloadingValue {
	public:
		NativeFnCallback callback;

		inline NativeFnOverloadingValue(
			FnValue *fnValue,
			AccessModifier access,
			std::deque<Type> paramTypes,
			Type returnType,
			NativeFnCallback callback)
			: FnOverloadingValue(
				  fnValue,
				  access,
				  paramTypes,
				  returnType),
			  callback(callback) {}
		virtual ~NativeFnOverloadingValue() = default;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const override;

		virtual FnOverloadingValue *duplicate() const override;

		inline NativeFnOverloadingValue &operator=(const NativeFnOverloadingValue &other) {
			*(FnOverloadingValue *)this = (const FnOverloadingValue &)other;

			callback = other.callback;

			return *this;
		}
	};

	class FnValue : public MemberValue {
	public:
		FnValue *parentFn = nullptr;
		std::deque<FnOverloadingValue *> overloadings;

		inline FnValue(Runtime *rt) : MemberValue(rt, ACCESS_PUB) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(MemberValue));
		}
		virtual inline ~FnValue() {
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(MemberValue));
		}

		virtual Type getType() const override;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args, std::deque<Type> argTypes) const override;

		virtual Value *duplicate() const override;

		inline FnValue &operator=(const FnValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;

			for (auto &i : x.overloadings) {
				FnOverloadingValue *ol = i->duplicate();

				ol->fnValue = this;

				overloadings.push_back(ol);
			}

			return *this;
		}
		FnValue &operator=(FnValue &&) = delete;
	};
}

#endif
