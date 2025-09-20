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

#include <peff/containers/map.h>

namespace slake {
	struct Context;
	struct MajorFrame;

	class Instruction final {
	public:
		Opcode opcode;
		uint32_t output;
		uint32_t nOperands;
		Value *operands;
		peff::RcObjectPtr<peff::Alloc> operandsAllocator;

		SLAKE_API Instruction();
		SLAKE_API Instruction(Instruction &&rhs);
		SLAKE_API ~Instruction();

		SLAKE_API bool operator==(const Instruction &rhs) const;
		SLAKE_FORCEINLINE bool operator!=(const Instruction &rhs) const {
			return !(*this == rhs);
		}

		SLAKE_API bool operator<(const Instruction &rhs) const;

		SLAKE_API Instruction &operator=(Instruction &&rhs);

		SLAKE_FORCEINLINE void setOpcode(Opcode opcode) {
			this->opcode = opcode;
		}

		SLAKE_FORCEINLINE void setOutput(uint32_t output) {
			this->output = output;
		}

		SLAKE_API void clearOperands();
		[[nodiscard]] SLAKE_API bool reserveOperands(peff::Alloc *allocator, uint32_t nOperands);

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
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

		AccessModifier access = 0;

		GenericParamList genericParams;
		peff::HashMap<peff::String, TypeRef> mappedGenericArgs;

		peff::DynArray<TypeRef> paramTypes;
		TypeRef returnType;

		OverloadingFlags overloadingFlags = 0;

		SLAKE_API FnOverloadingObject(
			FnOverloadingKind overloadingKind,
			FnObject *fnObject,
			peff::Alloc *selfAllocator);
		SLAKE_API FnOverloadingObject(const FnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~FnOverloadingObject();

		SLAKE_FORCEINLINE void setAccess(AccessModifier accessModifier) {
			this->access = accessModifier;
		}

		SLAKE_FORCEINLINE void setParamTypes(peff::DynArray<TypeRef> &&paramTypes) noexcept {
			this->paramTypes = std::move(paramTypes);
		}

		SLAKE_FORCEINLINE void setReturnType(TypeRef returnType) noexcept {
			this->returnType = returnType;
		}

		SLAKE_FORCEINLINE TypeRef getReturnType() noexcept {
			return returnType;
		}

		SLAKE_FORCEINLINE void setVarArgs() noexcept {
			overloadingFlags |= OL_VARG;
		}

		SLAKE_FORCEINLINE void clearVarArgs() noexcept {
			overloadingFlags &= ~OL_VARG;
		}

		SLAKE_FORCEINLINE bool isWithVarArgs() const noexcept {
			return overloadingFlags & OL_VARG;
		}

		SLAKE_FORCEINLINE void setCoroutine() noexcept {
			overloadingFlags |= OL_GENERATOR;
		}

		SLAKE_FORCEINLINE void clearCoroutine() noexcept {
			overloadingFlags &= ~OL_GENERATOR;
		}

		SLAKE_FORCEINLINE bool isCoroutine() const noexcept {
			return overloadingFlags & OL_GENERATOR;
		}

		SLAKE_FORCEINLINE void setVirtualFlag() noexcept {
			overloadingFlags |= OL_VIRTUAL;
		}

		SLAKE_FORCEINLINE void clearVirtualFlag() noexcept {
			overloadingFlags &= ~OL_VIRTUAL;
		}

		SLAKE_FORCEINLINE bool isVirtual() noexcept {
			return overloadingFlags & OL_VIRTUAL;
		}

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		peff::DynArray<slxfmt::SourceLocDesc> sourceLocDescs;
		peff::DynArray<Instruction> instructions;
		TypeRef thisType = TypeId::Void;
		uint32_t nRegisters;

		SLAKE_API RegularFnOverloadingObject(
			FnObject *fnObject,
			peff::Alloc *selfAllocator);
		SLAKE_API RegularFnOverloadingObject(const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~RegularFnOverloadingObject();

		SLAKE_API const slxfmt::SourceLocDesc *getSourceLocationDesc(uint32_t offIns) const;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fnObject);
		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE void setThisType(TypeRef thisType) noexcept {
			this->thisType = thisType;
		}

		SLAKE_FORCEINLINE TypeRef getThisType() noexcept {
			return thisType;
		}

		SLAKE_FORCEINLINE void setRegisterNumber(uint32_t nRegisters) noexcept {
			this->nRegisters = nRegisters;
		}

		SLAKE_FORCEINLINE uint32_t getRegisterNumber() noexcept {
			return nRegisters;
		}

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class JITCompiledFnOverloadingObject : public FnOverloadingObject {
	public:
		RegularFnOverloadingObject *uncompiledVersion;
		peff::Set<Object *> referencedObjects;

		SLAKE_API JITCompiledFnOverloadingObject(
			FnObject *fnObject,
			peff::Alloc *selfAllocator,
			AccessModifier access);
		SLAKE_API JITCompiledFnOverloadingObject(const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~JITCompiledFnOverloadingObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<JITCompiledFnOverloadingObject> alloc(
			FnObject *fnObject,
			AccessModifier access);
		SLAKE_API static HostObjectRef<JITCompiledFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class NativeFnOverloadingObject;
	using NativeFnCallback =
		std::function<Value(Context *context, MajorFrame *curMajorFrame)>;

	class NativeFnOverloadingObject : public FnOverloadingObject {
	public:
		NativeFnCallback callback;

		SLAKE_API NativeFnOverloadingObject(
			FnObject *fnObject,
			peff::Alloc *selfAllocator,
			NativeFnCallback callback);
		SLAKE_API NativeFnOverloadingObject(const NativeFnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~NativeFnOverloadingObject();

		SLAKE_API virtual FnOverloadingObject *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(
			FnObject *fnObject,
			NativeFnCallback callback);
		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(const NativeFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	struct FnSignature {
		const peff::DynArray<TypeRef> &paramTypes;
		bool hasVarArg;
		size_t nGenericParams;
	};

	struct FnSignatureComparator {
		GenericArgListComparator innerComparator;

		SLAKE_API int operator()(const FnSignature &lhs, const FnSignature &rhs) const noexcept;
	};

	struct FnSignatureLtComparator {
		FnSignatureComparator innerComparator;

		SLAKE_FORCEINLINE bool operator()(const FnSignature &lhs, const FnSignature &rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};

	class FnObject : public MemberObject {
	public:
		peff::Map<FnSignature, FnOverloadingObject *, FnSignatureLtComparator> overloadings;

		SLAKE_API FnObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API FnObject(const FnObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~FnObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<FnObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<FnObject> alloc(const FnObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;

		SLAKE_API InternalExceptionPointer resortOverloadings() noexcept;
	};

	SLAKE_API FnOverloadingObject *findOverloading(
		FnObject *fnObject,
		const peff::DynArray<TypeRef> &paramTypes,
		size_t nGenericParams,
		bool hasVarArg);
	SLAKE_API bool isDuplicatedOverloading(
		const FnOverloadingObject *overloading,
		const peff::DynArray<TypeRef> &paramTypes,
		size_t nGenericParams,
		bool hasVarArg);
}

#endif
