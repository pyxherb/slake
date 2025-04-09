#ifndef _SLAKE_OBJ_FN_H_
#define _SLAKE_OBJ_FN_H_

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
	struct MajorFrame;

	struct Instruction final {
		Opcode opcode = (Opcode)0xffff;
		uint32_t output = UINT32_MAX;
		Value operands[3];
		size_t nOperands;

		SLAKE_API bool operator==(const Instruction &rhs) const;
		SLAKE_FORCEINLINE bool operator!=(const Instruction &rhs) const {
			return !(*this == rhs);
		}

		SLAKE_API bool operator<(const Instruction &rhs) const;
	};

	enum class FnOverloadingKind {
		Regular = 0,
		Native,
		JITCompiled
	};

	using OverloadingFlags = uint32_t;

	constexpr static OverloadingFlags
		OL_VARG = 0x01,		  // Has varidic parameters
		OL_GENERATOR = 0x02,  // Is generator
		OL_VIRTUAL = 0x04,	  // Is virtual
		OL_PURE = 0x08		  // Is pure
		;

	class FnObject;

	class FnOverloadingObject : public Object {
	public:
		FnOverloadingKind overloadingKind;

		FnObject *fnObject;

		AccessModifier access;

		GenericParamList genericParams;
		peff::HashMap<peff::String, Type> mappedGenericArgs;

		GenericArgList specializationArgs;

		peff::DynArray<Type> paramTypes;
		Type returnType;

		OverloadingFlags overloadingFlags = 0;

		SLAKE_API FnOverloadingObject(
			FnOverloadingKind overloadingKind,
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			OverloadingFlags flags);
		SLAKE_API FnOverloadingObject(const FnOverloadingObject &other, bool &succeededOut);
		SLAKE_API virtual ~FnOverloadingObject();

		SLAKE_API virtual ObjectKind getKind() const;

		SLAKE_API virtual FnOverloadingObject *duplicate() const = 0;
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		peff::DynArray<slxfmt::SourceLocDesc> sourceLocDescs;
		peff::DynArray<Instruction> instructions;
		Type thisObjectType = TypeId::None;
		uint32_t nRegisters;

		SLAKE_API RegularFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			uint32_t nRegisters,
			OverloadingFlags flags);
		SLAKE_API RegularFnOverloadingObject(const RegularFnOverloadingObject &other, bool &succeededOut);
		SLAKE_API virtual ~RegularFnOverloadingObject();

		SLAKE_API const slxfmt::SourceLocDesc *getSourceLocationDesc(uint32_t offIns) const;

		SLAKE_API virtual FnOverloadingObject *duplicate() const override;

		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			uint32_t nRegisters,
			OverloadingFlags flags);
		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class JITCompiledFnOverloadingObject : public FnOverloadingObject {
	public:
		RegularFnOverloadingObject *uncompiledVersion;
		peff::Set<Object *> referencedObjects;

		SLAKE_API JITCompiledFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			uint32_t nRegisters,
			OverloadingFlags flags);
		SLAKE_API JITCompiledFnOverloadingObject(const RegularFnOverloadingObject &other, bool &succeededOut);
		SLAKE_API virtual ~JITCompiledFnOverloadingObject();

		SLAKE_API virtual FnOverloadingObject *duplicate() const override;

		SLAKE_API static HostObjectRef<JITCompiledFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			OverloadingFlags flags);
		SLAKE_API static HostObjectRef<JITCompiledFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class NativeFnOverloadingObject;
	using NativeFnCallback =
		std::function<Value(Context *context, MajorFrame *curMajorFrame)>;

	class NativeFnOverloadingObject : public FnOverloadingObject {
	public:
		SLAKE_API NativeFnOverloadingObject(
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			OverloadingFlags flags,
			NativeFnCallback callback);
		SLAKE_API NativeFnOverloadingObject(const NativeFnOverloadingObject &other, bool &succeededOut);
		SLAKE_API virtual ~NativeFnOverloadingObject();

		NativeFnCallback callback;

		SLAKE_API virtual FnOverloadingObject *duplicate() const override;

		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access,
			peff::DynArray<Type> &&paramTypes,
			const Type &returnType,
			OverloadingFlags flags,
			NativeFnCallback callback);
		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(const NativeFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class FnObject : public MemberObject {
	public:
		Object *parent = nullptr;
		peff::Set<FnOverloadingObject *> overloadings;

		SLAKE_API FnObject(Runtime *rt);
		SLAKE_API FnObject(const FnObject &x, bool &succeededOut);
		SLAKE_API virtual ~FnObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *getParent() const override;
		SLAKE_API virtual void setParent(Object *parent) override;

		SLAKE_API FnOverloadingObject *getOverloading(const peff::DynArray<Type> &argTypes) const;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<FnObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<FnObject> alloc(const FnObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	SLAKE_API FnOverloadingObject *findOverloading(
		FnObject *fnObject,
		const peff::DynArray<Type> &paramTypes,
		const GenericParamList &genericParams,
		bool hasVarArg);
	SLAKE_API bool isDuplicatedOverloading(
		const FnOverloadingObject *overloading,
		const peff::DynArray<Type> &paramTypes,
		const GenericParamList &genericParams,
		bool hasVarArg);
}

#endif
