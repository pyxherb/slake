#ifndef _SLAKE_VALDEF_FN_H_
#define _SLAKE_VALDEF_FN_H_

#include <slake/opcode.h>
#include <slake/slxfmt.h>

#include <functional>
#include <deque>
#include <vector>
#include <memory>

#include "member.h"
#include "generic.h"

namespace slake {
	struct Context;

	struct Instruction final {
		Opcode opcode = (Opcode)0xffff;
		Value output;
		std::vector<Value> operands;

		bool operator==(const Instruction &rhs) const;
		inline bool operator!=(const Instruction &rhs) const {
			return !(*this == rhs);
		}

		bool operator<(const Instruction &rhs) const;
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
		FnObject *fnObject;

		AccessModifier access;

		GenericParamList genericParams;
		std::unordered_map<std::string, Type> mappedGenericArgs;

		GenericArgList specializationArgs;

		std::pmr::vector<Type> paramTypes;
		Type returnType;

		OverloadingFlags overloadingFlags = 0;

		FnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType);
		inline FnOverloadingObject(const FnOverloadingObject &other) : Object(other) {
			fnObject = other.fnObject;

			access = other.access;

			genericParams = other.genericParams;
			mappedGenericArgs = other.mappedGenericArgs;

			paramTypes = other.paramTypes;
			returnType = other.returnType;

			overloadingFlags = other.overloadingFlags;
		}
		virtual ~FnOverloadingObject();

		virtual inline ObjectKind getKind() const { return ObjectKind::FnOverloading; }

		virtual FnOverloadingKind getOverloadingKind() const = 0;

		virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const = 0;

		virtual FnOverloadingObject *duplicate() const = 0;
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		std::vector<slxfmt::SourceLocDesc> sourceLocDescs;
		std::vector<Instruction> instructions;

		inline RegularFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType)
			: FnOverloadingObject(
				  fnObject,
				  access,
				  std::move(paramTypes),
				  returnType) {}
		inline RegularFnOverloadingObject(const RegularFnOverloadingObject &other) : FnOverloadingObject(other) {
			sourceLocDescs = other.sourceLocDescs;

			instructions.resize(other.instructions.size());
			for (size_t i = 0; i < instructions.size(); ++i) {
				instructions[i].opcode = other.instructions[i].opcode;

				if (auto &output = other.instructions[i].output; output.valueType == ValueType::ObjectRef) {
					if (auto ptr = output.getObjectRef(); ptr)
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
						if (auto ptr = operand.getObjectRef(); ptr)
							instructions[i].operands[j] =
								ptr->duplicate();
						else
							instructions[i].operands[j] = nullptr;
					} else
						instructions[i].operands[j] = operand;
				}
			}
		}

		inline const slxfmt::SourceLocDesc *getSourceLocationDesc(uint32_t offIns) const {
			const slxfmt::SourceLocDesc *curDesc = nullptr;

			for (auto &i : sourceLocDescs) {
				if ((offIns >= i.offIns) &&
					(offIns < i.offIns + i.nIns)) {
					if (curDesc) {
						if ((i.offIns >= curDesc->offIns) &&
							(i.nIns < curDesc->nIns))
							curDesc = &i;
					} else
						curDesc = &i;
				}
			}

			return curDesc;
		}
		virtual ~RegularFnOverloadingObject() = default;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const override;

		virtual FnOverloadingObject *duplicate() const override;

		static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType);
		static HostObjectRef<RegularFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		virtual void dealloc() override;
	};

	using NativeFnCallback =
		std::function<Value(
			Runtime *rt,
			Object *thisObject,
			std::pmr::vector<Value> args,
			const std::unordered_map<std::string, Type> &mappedGenericArgs)>;

	class NativeFnOverloadingObject : public FnOverloadingObject {
	public:
		inline NativeFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType,
			NativeFnCallback callback)
			: FnOverloadingObject(
				  fnObject,
				  access,
				  std::move(paramTypes),
				  returnType),
			  callback(callback) {}
		inline NativeFnOverloadingObject(const NativeFnOverloadingObject &other) : FnOverloadingObject(other) {
			callback = other.callback;
		}
		virtual ~NativeFnOverloadingObject() = default;

		NativeFnCallback callback;

		virtual FnOverloadingKind getOverloadingKind() const override;

		virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const override;

		virtual FnOverloadingObject *duplicate() const override;

		static HostObjectRef<NativeFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			const std::vector<Type> &paramTypes,
			const Type &returnType,
			NativeFnCallback callback);
		static HostObjectRef<NativeFnOverloadingObject> alloc(const NativeFnOverloadingObject *other);
		virtual void dealloc() override;
	};

	class FnObject : public MemberObject {
	public:
		std::string name;
		Object *parent = nullptr;

		inline FnObject(Runtime *rt) : MemberObject(rt) {
		}
		inline FnObject(const FnObject &x) : MemberObject(x) {
			for (auto &i : x.overloadings) {
				FnOverloadingObject *ol = i->duplicate();

				ol->fnObject = this;

				overloadings.insert(ol);
			}

			parent = x.parent;
		}
		virtual inline ~FnObject() {
		}

		std::set<FnOverloadingObject *> overloadings;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Fn; }

		virtual inline const char *getName() const override { return name.c_str(); }
		virtual void setName(const char *name) override { this->name = name; }
		virtual inline Object *getParent() const override { return parent; }
		virtual void setParent(Object *parent) override { this->parent = parent; }

		FnOverloadingObject *getOverloading(const std::pmr::vector<Type> &argTypes) const;

		virtual Value call(Object *thisObject, std::pmr::vector<Value> args, std::pmr::vector<Type> argTypes, HostRefHolder *hostRefHolder) const;

		virtual Object *duplicate() const override;

		static HostObjectRef<FnObject> alloc(Runtime *rt);
		static HostObjectRef<FnObject> alloc(const FnObject *other);
		virtual void dealloc() override;
	};

	inline bool isDuplicatedOverloading(
		const FnOverloadingObject *overloading,
		const std::pmr::vector<Type> &paramTypes,
		const GenericParamList &genericParams,
		bool hasVarArg) {
		if ((overloading->overloadingFlags & OL_VARG) != (hasVarArg ? OL_VARG : 0))
			return false;

		if (overloading->paramTypes.size() != paramTypes.size())
			return false;

		if (overloading->genericParams.size() != genericParams.size())
			return false;

		for (size_t j = 0; j < paramTypes.size(); ++j) {
			if (overloading->paramTypes[j] != paramTypes[j])
				return false;
		}

		return true;
	}
}

#endif
