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
		Value output;
		std::deque<Value> operands;
	};

	enum class FnOverloadingKind {
		Regular = 0,
		Native
	};

	using OverloadingFlags = uint32_t;

	constexpr static OverloadingFlags
		OL_VARG = 0x01,	   // Has varidic parameters
		OL_ASYNC = 0x02,   // Is asynchronous
		OL_VIRTUAL = 0x04  // Is virtual
		;

	class FnObject;

	class FnOverloadingObject : public Object {
	public:
		FnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			const std::deque<Type> &paramTypes,
			const Type &returnType);
		virtual ~FnOverloadingObject();

		FnObject *fnObject;

		AccessModifier access;

		GenericParamList genericParams;
		std::unordered_map<std::string, Type> mappedGenericArgs;

		std::deque<Type> paramTypes;
		Type returnType;

		OverloadingFlags overloadingFlags = 0;

		virtual inline Type getType() const { return TypeId::FnOverloading; }

		virtual FnOverloadingKind getOverloadingKind() const = 0;

		virtual Value call(Object *thisObject, std::deque<Value> args) const = 0;

		virtual FnOverloadingObject *duplicate() const = 0;

		inline FnOverloadingObject &operator=(const FnOverloadingObject &other) {
			fnObject = other.fnObject;

			access = other.access;

			genericParams = other.genericParams;
			mappedGenericArgs = other.mappedGenericArgs;

			paramTypes = other.paramTypes;
			returnType = other.returnType;

			overloadingFlags = other.overloadingFlags;

			return *this;
		}
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		inline RegularFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			const std::deque<Type> &paramTypes,
			const Type &returnType)
			: FnOverloadingObject(
				  fnObject,
				  access,
				  paramTypes,
				  returnType) {}
		virtual ~RegularFnOverloadingObject() = default;

		std::deque<slxfmt::SourceLocDesc> sourceLocDescs;
		std::deque<Instruction> instructions;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual Value call(Object *thisObject, std::deque<Value> args) const override;

		virtual FnOverloadingObject *duplicate() const override;

		static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			const std::deque<Type> &paramTypes,
			const Type &returnType);
		virtual void dealloc() override;

		inline RegularFnOverloadingObject &operator=(const RegularFnOverloadingObject &other) {
			*(FnOverloadingObject *)this = (const FnOverloadingObject &)other;

			sourceLocDescs = other.sourceLocDescs;

			instructions.resize(other.instructions.size());
			for (size_t i = 0; i < instructions.size(); ++i) {
				instructions[i].opcode = other.instructions[i].opcode;

				if (auto &output = other.instructions[i].output; output.valueType == ValueType::ObjectRef) {
					if (auto ptr = output.getObjectRef().objectPtr; ptr)
						instructions[i].output = ptr->duplicate();
					else
						instructions[i].output = nullptr;
				} else
					instructions[i].output = output;

				// Duplicate each of the operands.
				instructions[i].operands.resize(other.instructions[i].operands.size());
				for (size_t j = 0; j < other.instructions[i].operands.size(); ++j) {
					auto &operand = other.instructions[i].operands[j];

					if (operand.valueType == ValueType::ObjectRef) {
						if (auto ptr = operand.getObjectRef().objectPtr; ptr)
							instructions[i].operands[j] =
								ptr->duplicate();
						else
							instructions[i].operands[j] = nullptr;
					} else
						instructions[i].operands[j] = operand;
				}
			}

			return *this;
		}
	};

	using NativeFnCallback =
		std::function<Value(
			Runtime *rt,
			Object *thisObject,
			std::deque<Value> args,
			const std::unordered_map<std::string, Type> &mappedGenericArgs)>;

	class NativeFnOverloadingObject : public FnOverloadingObject {
	public:
		inline NativeFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			const std::deque<Type> &paramTypes,
			const Type &returnType,
			NativeFnCallback callback)
			: FnOverloadingObject(
				  fnObject,
				  access,
				  paramTypes,
				  returnType),
			  callback(callback) {}
		virtual ~NativeFnOverloadingObject() = default;

		NativeFnCallback callback;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual Value call(Object *thisObject, std::deque<Value> args) const override;

		virtual FnOverloadingObject *duplicate() const override;

		static HostObjectRef<NativeFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			const std::deque<Type> &paramTypes,
			const Type &returnType,
			NativeFnCallback callback);
		virtual void dealloc() override;

		inline NativeFnOverloadingObject &operator=(const NativeFnOverloadingObject &other) {
			*(FnOverloadingObject *)this = (const FnOverloadingObject &)other;

			callback = other.callback;

			return *this;
		}
	};

	class FnObject : public MemberObject {
	public:
		inline FnObject(Runtime *rt) : MemberObject(rt, ACCESS_PUB) {
		}
		virtual inline ~FnObject() {
		}

		FnObject *parentFn = nullptr, *descentFn = nullptr;
		std::deque<FnOverloadingObject *> overloadings;

		virtual Type getType() const override;

		FnOverloadingObject *getOverloading(std::deque<Type> argTypes) const;

		virtual Value call(Object *thisObject, std::deque<Value> args, std::deque<Type> argTypes) const;

		virtual Object *duplicate() const override;

		static HostObjectRef<FnObject> alloc(Runtime *rt);
		virtual void dealloc() override;

		inline FnObject &operator=(const FnObject &x) {
			((MemberObject &)*this) = (MemberObject &)x;

			// We don't copy parentFn because it has to be linked to a inherited function dynamically LOL

			for (auto &i : x.overloadings) {
				FnOverloadingObject *ol = i->duplicate();

				ol->fnObject = this;

				overloadings.push_back(ol);
			}

			return *this;
		}
		FnObject &operator=(FnObject &&) = delete;
	};
}

#endif
