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

	class FnOverloading {
	public:
		FnValue *fnValue;

		AccessModifier access;

		GenericParamList genericParams;
		std::unordered_map<std::string, Type> mappedGenericArgs;

		std::deque<Type> paramTypes;
		Type returnType;

		OverloadingFlags overloadingFlags = 0;

		inline FnOverloading(
			FnValue *fnValue,
			AccessModifier access,
			std::deque<Type> paramTypes,
			Type returnType)
			: fnValue(fnValue),
			  access(access),
			  paramTypes(paramTypes),
			  returnType(returnType) {}
		virtual ~FnOverloading() = default;

		virtual FnOverloadingKind getOverloadingKind() const = 0;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const = 0;

		virtual FnOverloading *duplicate() const = 0;

		inline FnOverloading &operator=(const FnOverloading &other) {
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

	class RegularFnOverloading : public FnOverloading {
	public:
		std::deque<slxfmt::SourceLocDesc> sourceLocDescs;
		std::deque<Instruction> instructions;

		inline RegularFnOverloading(
			FnValue *fnValue,
			AccessModifier access,
			std::deque<Type> paramTypes,
			Type returnType)
			: FnOverloading(
				  fnValue,
				  access,
				  paramTypes,
				  returnType) {}
		virtual ~RegularFnOverloading() = default;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const override;

		virtual FnOverloading *duplicate() const override;

		inline RegularFnOverloading &operator=(const RegularFnOverloading &other) {
			*(FnOverloading *)this = (const FnOverloading &)other;

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

	class NativeFnOverloading : public FnOverloading {
	public:
		NativeFnCallback callback;

		inline NativeFnOverloading(
			FnValue *fnValue,
			AccessModifier access,
			std::deque<Type> paramTypes,
			Type returnType,
			NativeFnCallback callback)
			: FnOverloading(
				  fnValue,
				  access,
				  paramTypes,
				  returnType),
			  callback(callback) {}
		virtual ~NativeFnOverloading() = default;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const override;

		virtual FnOverloading *duplicate() const override;

		inline NativeFnOverloading &operator=(const NativeFnOverloading &other) {
			*(FnOverloading *)this = (const FnOverloading &)other;

			callback = other.callback;

			return *this;
		}
	};

	class FnValue : public MemberValue {
	public:
		std::deque<std::unique_ptr<FnOverloading>> overloadings;

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
				std::unique_ptr<FnOverloading> ol(i->duplicate());

				ol->fnValue = this;

				overloadings.push_back(std::move(ol));
			}

			return *this;
		}
		FnValue &operator=(FnValue &&) = delete;
	};
}

#endif
