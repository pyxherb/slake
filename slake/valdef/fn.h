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

		SLAKE_API bool operator==(const Instruction &rhs) const;
		SLAKE_FORCEINLINE bool operator!=(const Instruction &rhs) const {
			return !(*this == rhs);
		}

		SLAKE_API bool operator<(const Instruction &rhs) const;
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

		SLAKE_API FnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType);
		SLAKE_API FnOverloadingObject(const FnOverloadingObject &other);
		SLAKE_API virtual ~FnOverloadingObject();

		SLAKE_API virtual ObjectKind getKind() const;

		SLAKE_API virtual FnOverloadingKind getOverloadingKind() const = 0;

		SLAKE_API virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const = 0;

		SLAKE_API virtual FnOverloadingObject *duplicate() const = 0;
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		std::vector<slxfmt::SourceLocDesc> sourceLocDescs;
		std::vector<Instruction> instructions;

		SLAKE_API RegularFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType);
		SLAKE_API RegularFnOverloadingObject(const RegularFnOverloadingObject &other);
		SLAKE_API virtual ~RegularFnOverloadingObject();

		SLAKE_API const slxfmt::SourceLocDesc *getSourceLocationDesc(uint32_t offIns) const;

		SLAKE_API virtual FnOverloadingKind getOverloadingKind() const override;

		SLAKE_API virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const override;

		SLAKE_API virtual FnOverloadingObject *duplicate() const override;

		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType);
		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	using NativeFnCallback =
		std::function<Value(
			Runtime *rt,
			Object *thisObject,
			std::pmr::vector<Value> args,
			const std::unordered_map<std::string, Type> &mappedGenericArgs)>;

	class NativeFnOverloadingObject : public FnOverloadingObject {
	public:
		SLAKE_API NativeFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType,
			NativeFnCallback callback);
		SLAKE_API NativeFnOverloadingObject(const NativeFnOverloadingObject &other);
		SLAKE_API virtual ~NativeFnOverloadingObject();

		NativeFnCallback callback;

		SLAKE_API virtual FnOverloadingKind getOverloadingKind() const override;

		SLAKE_API virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const override;

		SLAKE_API virtual FnOverloadingObject *duplicate() const override;

		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			const std::vector<Type> &paramTypes,
			const Type &returnType,
			NativeFnCallback callback);
		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(const NativeFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class FnObject : public MemberObject {
	public:
		std::string name;
		Object *parent = nullptr;

		SLAKE_API FnObject(Runtime *rt);
		SLAKE_API FnObject(const FnObject &x);
		SLAKE_API virtual ~FnObject();

		std::set<FnOverloadingObject *> overloadings;

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual const char *getName() const override;
		SLAKE_API virtual void setName(const char *name) override;
		SLAKE_API virtual Object *getParent() const override;
		SLAKE_API virtual void setParent(Object *parent) override;

		SLAKE_API FnOverloadingObject *getOverloading(const std::pmr::vector<Type> &argTypes) const;

		SLAKE_API virtual Value call(Object *thisObject, std::pmr::vector<Value> args, std::pmr::vector<Type> argTypes, HostRefHolder *hostRefHolder) const;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<FnObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<FnObject> alloc(const FnObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	SLAKE_API bool isDuplicatedOverloading(
		const FnOverloadingObject *overloading,
		const std::pmr::vector<Type> &paramTypes,
		const GenericParamList &genericParams,
		bool hasVarArg);
}

#endif
