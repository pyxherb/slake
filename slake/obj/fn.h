#ifndef _SLAKE_OBJ_FN_H_
#define _SLAKE_OBJ_FN_H_

#include <slake/opcode.h>
#include <slake/slxfmt.h>

#include <functional>

#include "member.h"
#include "generic.h"

#include <peff/containers/map.h>
#include <peff/containers/hashmap.h>

namespace slake {
	struct Context;
	struct MajorFrame;

	class Instruction final {
	public:
		size_t off_source_loc_desc = SIZE_MAX;
		Opcode opcode;
		uint32_t output;
		uint32_t num_operands;
		Value *operands;
		peff::RcObjectPtr<peff::Alloc> operands_allocator;

		SLAKE_API Instruction();
		SLAKE_API Instruction(Instruction &&rhs);
		SLAKE_API ~Instruction();

		SLAKE_API bool operator==(const Instruction &rhs) const;
		SLAKE_FORCEINLINE bool operator!=(const Instruction &rhs) const {
			return !(*this == rhs);
		}

		SLAKE_API bool operator<(const Instruction &rhs) const;

		SLAKE_API Instruction &operator=(Instruction &&rhs);

		SLAKE_FORCEINLINE void set_opcode(Opcode opcode) {
			this->opcode = opcode;
		}

		SLAKE_FORCEINLINE void set_output(uint32_t output) {
			this->output = output;
		}

		SLAKE_API void clear_operands();
		[[nodiscard]] SLAKE_API bool reserve_operands(peff::Alloc *allocator, uint32_t num_operands);

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
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
		FnOverloadingKind overloading_kind;

		FnObject *fn_object;

		AccessModifier access = 0;

		GenericParamList generic_params;
		peff::HashMap<std::string_view, size_t> mapped_generic_params;
		peff::HashMap<std::string_view, TypeRef> mapped_generic_args;

		peff::DynArray<TypeRef> param_types;
		TypeRef return_type;

		TypeRef overriden_type;

		OverloadingFlags overloading_flags = 0;

		SLAKE_API FnOverloadingObject(
			FnOverloadingKind overloading_kind,
			FnObject *fn_object,
			peff::Alloc *self_allocator);
		SLAKE_API FnOverloadingObject(const FnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~FnOverloadingObject();

		SLAKE_FORCEINLINE void set_access(AccessModifier access_modifier) {
			this->access = access_modifier;
		}

		SLAKE_FORCEINLINE void set_param_types(peff::DynArray<TypeRef> &&param_types) noexcept {
			this->param_types = std::move(param_types);
		}

		SLAKE_FORCEINLINE void set_return_type(TypeRef return_type) noexcept {
			this->return_type = return_type;
		}

		SLAKE_FORCEINLINE TypeRef get_return_type() noexcept {
			return return_type;
		}

		SLAKE_FORCEINLINE void set_var_args() noexcept {
			overloading_flags |= OL_VARG;
		}

		SLAKE_FORCEINLINE void clear_var_args() noexcept {
			overloading_flags &= ~OL_VARG;
		}

		SLAKE_FORCEINLINE bool is_with_var_args() const noexcept {
			return overloading_flags & OL_VARG;
		}

		SLAKE_FORCEINLINE void set_coroutine() noexcept {
			overloading_flags |= OL_GENERATOR;
		}

		SLAKE_FORCEINLINE void clear_coroutine() noexcept {
			overloading_flags &= ~OL_GENERATOR;
		}

		SLAKE_FORCEINLINE bool is_coroutine() const noexcept {
			return overloading_flags & OL_GENERATOR;
		}

		SLAKE_FORCEINLINE void set_virtual_flag() noexcept {
			overloading_flags |= OL_VIRTUAL;
		}

		SLAKE_FORCEINLINE void clear_virtual_flag() noexcept {
			overloading_flags &= ~OL_VIRTUAL;
		}

		SLAKE_FORCEINLINE bool is_virtual() noexcept {
			return overloading_flags & OL_VIRTUAL;
		}

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class RegularFnOverloadingObject : public FnOverloadingObject {
	public:
		peff::DynArray<slxfmt::SourceLocDesc> source_loc_descs;
		peff::DynArray<Instruction> instructions;
		TypeRef this_type = TypeId::Void;
		uint32_t num_registers;

		SLAKE_API RegularFnOverloadingObject(
			FnObject *fn_object,
			peff::Alloc *self_allocator);
		SLAKE_API RegularFnOverloadingObject(const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~RegularFnOverloadingObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fn_object);
		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE void set_this_type(TypeRef this_type) noexcept {
			this->this_type = this_type;
		}

		SLAKE_FORCEINLINE TypeRef get_this_type() noexcept {
			return this_type;
		}

		SLAKE_FORCEINLINE void set_register_number(uint32_t num_registers) noexcept {
			this->num_registers = num_registers;
		}

		SLAKE_FORCEINLINE uint32_t get_register_number() noexcept {
			return num_registers;
		}

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class JITCompiledFnOverloadingObject : public FnOverloadingObject {
	public:
		RegularFnOverloadingObject *uncompiled_version;
		peff::Set<Object *> referenced_objects;

		SLAKE_API JITCompiledFnOverloadingObject(
			FnObject *fn_object,
			peff::Alloc *self_allocator,
			AccessModifier access);
		SLAKE_API JITCompiledFnOverloadingObject(const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~JITCompiledFnOverloadingObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<JITCompiledFnOverloadingObject> alloc(
			FnObject *fn_object,
			AccessModifier access);
		SLAKE_API static HostObjectRef<JITCompiledFnOverloadingObject> alloc(const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class NativeFnOverloadingObject;
	using NativeFnCallback =
		std::function<Value(Context *context, MajorFrame *cur_major_frame)>;

	class NativeFnOverloadingObject : public FnOverloadingObject {
	public:
		NativeFnCallback callback;

		SLAKE_API NativeFnOverloadingObject(
			FnObject *fn_object,
			peff::Alloc *self_allocator,
			NativeFnCallback callback);
		SLAKE_API NativeFnOverloadingObject(const NativeFnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~NativeFnOverloadingObject();

		SLAKE_API virtual FnOverloadingObject *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(
			FnObject *fn_object,
			NativeFnCallback callback);
		SLAKE_API static HostObjectRef<NativeFnOverloadingObject> alloc(const NativeFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	struct FnSignature {
		const peff::DynArray<TypeRef> &param_types;
		bool has_var_arg;
		size_t num_generic_params;
		TypeRef overriden_type;

		SLAKE_FORCEINLINE FnSignature(const peff::DynArray<TypeRef> &param_types, bool has_var_arg, size_t num_generic_params, const TypeRef &overriden_type) : param_types(param_types), has_var_arg(has_var_arg), num_generic_params(num_generic_params), overriden_type(overriden_type) {}
	};

	struct FnSignatureComparator {
		ParamListComparator inner_comparator;

		SLAKE_API int operator()(const FnSignature &lhs, const FnSignature &rhs) const noexcept;
	};

	struct FnSignatureLtComparator {
		FnSignatureComparator inner_comparator;

		SLAKE_FORCEINLINE bool operator()(const FnSignature &lhs, const FnSignature &rhs) const noexcept {
			return inner_comparator(lhs, rhs) < 0;
		}
	};

	class FnObject : public MemberObject {
	public:
		peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true> overloadings;

		SLAKE_API FnObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API FnObject(const FnObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~FnObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<FnObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<FnObject> alloc(const FnObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;

		SLAKE_API InternalExceptionPointer resort_overloadings() noexcept;
	};
}

#endif
