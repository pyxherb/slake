#ifndef _SLAKE_OBJ_FN_H_
#define _SLAKE_OBJ_FN_H_

#include <slake/opcode.h>
#include <slake/slxfmt.h>

#include <functional>

#include "member.h"
#include "generic.h"

#include <peff/containers/map.h>
#include <peff/containers/hashmap.h>
#include <peff/utils/bitops.h>

namespace slake {
	struct Context;
	struct MajorFrame;

	enum class InsRegType : uint8_t {
		I8 = 0,
		I16,
		I32,
		I64,
		ISize,
		U8,
		U16,
		U32,
		U64,
		USize,
		F32,
		F64,
		Bool,
		Object,
		Any,

		MAX_VALUE
	};

	constexpr uint8_t INS_OP0_REG = 0x01,
					  INS_OP1_REG = 0x02;

	struct Instruction final {
		Opcode opcode;
		uint8_t flags : 4;
		uint8_t reg_out_type : 4;
		uint8_t reg0_type : 4;
		uint8_t reg1_type : 4;
		uint32_t reg_out;
		uint32_t reg0;
		uint32_t reg1;
		uint64_t operands[2];
	};

	SLAKE_FORCEINLINE int8_t ins_operand_as_i8(uint64_t operand) noexcept {
		return static_cast<int8_t>(peff::bit_cast<int64_t>(operand));
	}

	SLAKE_FORCEINLINE int16_t ins_operand_as_i16(uint64_t operand) noexcept {
		return static_cast<int16_t>(peff::bit_cast<int64_t>(operand));
	}

	SLAKE_FORCEINLINE int32_t ins_operand_as_i32(uint64_t operand) noexcept {
		return static_cast<int32_t>(peff::bit_cast<int64_t>(operand));
	}

	SLAKE_FORCEINLINE int64_t ins_operand_as_i64(uint64_t operand) noexcept {
		return static_cast<int64_t>(peff::bit_cast<int64_t>(operand));
	}

	SLAKE_FORCEINLINE ptrdiff_t ins_operand_as_isize(uint64_t operand) noexcept {
		return static_cast<ptrdiff_t>(peff::bit_cast<int64_t>(operand));
	}

	SLAKE_FORCEINLINE uint8_t ins_operand_as_u8(uint64_t operand) noexcept {
		return static_cast<uint8_t>(operand);
	}

	SLAKE_FORCEINLINE uint16_t ins_operand_as_u16(uint64_t operand) noexcept {
		return static_cast<uint16_t>(operand);
	}

	SLAKE_FORCEINLINE uint32_t ins_operand_as_u32(uint64_t operand) noexcept {
		return static_cast<uint32_t>(operand);
	}

	SLAKE_FORCEINLINE RegIndex ins_operand_as_reg_index(uint64_t operand) noexcept {
		return static_cast<RegIndex>(operand);
	}

	SLAKE_FORCEINLINE uint64_t ins_operand_as_u64(uint64_t operand) noexcept {
		return static_cast<uint64_t>(operand);
	}

	SLAKE_FORCEINLINE size_t ins_operand_as_usize(uint64_t operand) noexcept {
		return static_cast<size_t>(operand);
	}

	SLAKE_FORCEINLINE float ins_operand_as_f32(uint64_t operand) noexcept {
		return peff::bit_cast<float>(static_cast<uint32_t>(operand));
	}

	SLAKE_FORCEINLINE double ins_operand_as_f64(uint64_t operand) noexcept {
		return peff::bit_cast<double>(operand);
	}

	SLAKE_FORCEINLINE bool ins_operand_as_bool(uint64_t operand) noexcept {
		return operand;
	}

	SLAKE_FORCEINLINE uint64_t i8_as_ins_operand(int8_t value) noexcept {
		return peff::bit_cast<uint64_t>(static_cast<int64_t>(value));
	}

	SLAKE_FORCEINLINE uint64_t i16_as_ins_operand(int16_t value) noexcept {
		return peff::bit_cast<uint64_t>(static_cast<int64_t>(value));
	}

	SLAKE_FORCEINLINE uint64_t i32_as_ins_operand(int32_t value) noexcept {
		return peff::bit_cast<uint64_t>(static_cast<int64_t>(value));
	}

	SLAKE_FORCEINLINE uint64_t i64_as_ins_operand(int64_t value) noexcept {
		return peff::bit_cast<uint64_t>(value);
	}

	SLAKE_FORCEINLINE uint64_t isize_as_ins_operand(ptrdiff_t value) noexcept {
		return peff::bit_cast<uint64_t>(static_cast<int64_t>(value));
	}

	SLAKE_FORCEINLINE uint64_t u8_as_ins_operand(uint8_t value) noexcept {
		return static_cast<uint64_t>(value);
	}

	SLAKE_FORCEINLINE uint64_t u16_as_ins_operand(uint16_t value) noexcept {
		return static_cast<uint64_t>(value);
	}

	SLAKE_FORCEINLINE uint64_t u32_as_ins_operand(uint32_t value) noexcept {
		return static_cast<uint64_t>(value);
	}

	SLAKE_FORCEINLINE uint64_t u64_as_ins_operand(uint64_t value) noexcept {
		return value;
	}

	SLAKE_FORCEINLINE uint64_t usize_as_ins_operand(size_t value) noexcept {
		return static_cast<uint64_t>(value);
	}

	SLAKE_FORCEINLINE uint64_t f32_as_ins_operand(float value) noexcept {
		return static_cast<uint64_t>(peff::bit_cast<uint32_t>(value));
	}

	SLAKE_FORCEINLINE uint64_t f64_as_ins_operand(double value) noexcept {
		return peff::bit_cast<uint64_t>(value);
	}

	SLAKE_FORCEINLINE uint64_t bool_as_ins_operand(bool value) noexcept {
		return static_cast<uint64_t>(value != 0);
	}

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
		uint32_t num_registers[(size_t)InsRegType::MAX_VALUE];
		peff::DynArray<Object *> ins_object_set;
		peff::DynArray<TypeRef> ins_type_set;

		SLAKE_API RegularFnOverloadingObject(
			FnObject *fn_object,
			peff::Alloc *self_allocator);
		SLAKE_API RegularFnOverloadingObject(Duplicator *duplicator, const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~RegularFnOverloadingObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(
			FnObject *fn_object);
		SLAKE_API static HostObjectRef<RegularFnOverloadingObject> alloc(
			Duplicator *duplicator,
			const RegularFnOverloadingObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE void set_this_type(TypeRef this_type) noexcept {
			this->this_type = this_type;
		}

		SLAKE_FORCEINLINE TypeRef get_this_type() noexcept {
			return this_type;
		}

		SLAKE_FORCEINLINE void set_register_number(InsRegType type, uint32_t num_registers) noexcept {
			this->num_registers[static_cast<uint8_t>(type)] = num_registers;
		}

		SLAKE_FORCEINLINE uint32_t get_register_number(InsRegType type) noexcept {
			return num_registers[static_cast<uint8_t>(type)];
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
