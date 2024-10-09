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
		SLAKE_FORCEINLINE bool operator!=(const Instruction &rhs) const {
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
		FnOverloadingObject(const FnOverloadingObject &other);
		virtual ~FnOverloadingObject();

		virtual ObjectKind getKind() const;

		virtual FnOverloadingKind getOverloadingKind() const = 0;

		virtual Value call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const = 0;

		virtual FnOverloadingObject *duplicate() const = 0;
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		std::vector<slxfmt::SourceLocDesc> sourceLocDescs;
		std::vector<Instruction> instructions;

		RegularFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType);
		RegularFnOverloadingObject(const RegularFnOverloadingObject &other);
		virtual ~RegularFnOverloadingObject();

		const slxfmt::SourceLocDesc *getSourceLocationDesc(uint32_t offIns) const;

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
		NativeFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			std::pmr::vector<Type> &&paramTypes,
			const Type &returnType,
			NativeFnCallback callback);
		NativeFnOverloadingObject(const NativeFnOverloadingObject &other);
		virtual ~NativeFnOverloadingObject();

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

		FnObject(Runtime *rt);
		FnObject(const FnObject &x);
		virtual ~FnObject();

		std::set<FnOverloadingObject *> overloadings;

		virtual ObjectKind getKind() const override;

		virtual const char *getName() const override;
		virtual void setName(const char *name) override;
		virtual Object *getParent() const override;
		virtual void setParent(Object *parent) override;

		FnOverloadingObject *getOverloading(const std::pmr::vector<Type> &argTypes) const;

		virtual Value call(Object *thisObject, std::pmr::vector<Value> args, std::pmr::vector<Type> argTypes, HostRefHolder *hostRefHolder) const;

		virtual Object *duplicate() const override;

		static HostObjectRef<FnObject> alloc(Runtime *rt);
		static HostObjectRef<FnObject> alloc(const FnObject *other);
		virtual void dealloc() override;
	};

	bool isDuplicatedOverloading(
		const FnOverloadingObject *overloading,
		const std::pmr::vector<Type> &paramTypes,
		const GenericParamList &genericParams,
		bool hasVarArg);
}

#endif
