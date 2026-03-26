#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"
#include <variant>

namespace slake {
	namespace opti {
		struct RegLifetime {
			uint32_t off_begin_ins;
			uint32_t off_end_ins;
		};

		enum class RegStorageType : uint8_t {
			None = 0,		   // Unrecognized, may from indirect access
			FieldVar,		   // Fields
			InstanceFieldVar,  // Instance fields
			LocalVar,		   // Local variables
			ArgRef,			   // Arguments
			ArrayElement,	   // Array element
		};

		struct FieldVarRegStorageInfo {
			bool is_used_for_output;
		};

		struct LocalVarRegStorageInfo {
			bool is_used_for_output;
			/// @brief Register where the local variable is defined and received
			uint32_t definition_reg;
		};

		struct ArgRefRegStorageInfo {
			bool is_used_for_output;
			uint32_t idx_arg;
		};

		struct RegAnalyzedInfo {
			RegLifetime lifetime;
			peff::Set<uint32_t> use_points;
			TypeRef type;
			Value expected_value = Value(ValueType::Undefined);
			union {
				FieldVarRegStorageInfo as_field_var;
				LocalVarRegStorageInfo as_local_var;
				ArgRefRegStorageInfo as_arg_ref;
			} storage_info;
			RegStorageType storage_type = RegStorageType::None;

			SLAKE_FORCEINLINE RegAnalyzedInfo(peff::Alloc *allocator) : use_points(allocator) {}
		};

		struct LocalVarAnalyzedInfo {
			TypeRef type;
		};

		struct FnCallAnalyzedInfo {
			FnOverloadingObject *fn_object = nullptr;
			peff::DynArray<uint32_t> arg_push_ins_offs;

			SLAKE_FORCEINLINE FnCallAnalyzedInfo(peff::Alloc *self_allocator)
				: arg_push_ins_offs(self_allocator) {
			}
		};

		struct ProgramAnalyzedInfo {
			peff::RcObjectPtr<peff::Alloc> resource_allocator;
			HostObjectRef<ContextObject> context_object;
			peff::Map<uint32_t, RegAnalyzedInfo> analyzed_reg_info;
			peff::Map<uint32_t, FnCallAnalyzedInfo> analyzed_fn_call_info;
			peff::Map<FnOverloadingObject *, peff::DynArray<uint32_t>> fn_call_map;
			peff::Set<uint32_t> code_block_boundaries;

			SLAKE_FORCEINLINE ProgramAnalyzedInfo(Runtime *runtime, peff::Alloc *resource_allocator)
				: analyzed_reg_info(resource_allocator),
				  analyzed_fn_call_info(resource_allocator),
				  fn_call_map(resource_allocator),
				  code_block_boundaries(resource_allocator),
				  resource_allocator(resource_allocator) {
			}
		};

		struct ProgramAnalyzeContext {
			Runtime *runtime;
			peff::RcObjectPtr<peff::Alloc> resource_allocator;
			RegularFnOverloadingObject *fn_object;
			ProgramAnalyzedInfo &analyzed_info_out;
			HostRefHolder &host_ref_holder;
			uint32_t idx_cur_ins = 0;
			Object *last_call_target;
			TypeRef last_call_target_type;
			peff::DynArray<uint32_t> arg_push_ins_offs;

			SLAKE_FORCEINLINE ProgramAnalyzeContext(
				Runtime *runtime,
				peff::Alloc *resource_allocator,
				RegularFnOverloadingObject *fn_object,
				ProgramAnalyzedInfo &analyzed_info_out,
				HostRefHolder &host_ref_holder)
				: runtime(runtime),
				  resource_allocator(resource_allocator),
				  fn_object(fn_object),
				  analyzed_info_out(analyzed_info_out),
				  host_ref_holder(host_ref_holder),
				  arg_push_ins_offs(resource_allocator) {
			}
		};

		bool is_ins_has_side_effect(Opcode opcode);
		bool is_ins_simplifiable(Opcode opcode);

		void mark_reg_as_for_output(ProgramAnalyzeContext &analyze_context, uint32_t i);
		InternalExceptionPointer wrap_into_heap_type(
			Runtime *runtime,
			TypeRef type,
			HostRefHolder &host_ref_holder,
			HeapTypeObject *&heap_type_out);
		InternalExceptionPointer wrap_into_ref_type(
			Runtime *runtime,
			TypeRef type,
			HostRefHolder &host_ref_holder,
			TypeRef &type_out);
		InternalExceptionPointer wrap_into_array_type(
			Runtime *runtime,
			TypeRef type,
			HostRefHolder &host_ref_holder,
			TypeRef &type_out);
		InternalExceptionPointer eval_object_type(
			ProgramAnalyzeContext &analyze_context,
			const Reference &entity_ref,
			TypeRef &type_out);
		InternalExceptionPointer eval_value_type(
			ProgramAnalyzeContext &analyze_context,
			const Value &value,
			TypeRef &type_out);
		InternalExceptionPointer eval_const_value(
			ProgramAnalyzeContext &analyze_context,
			const Value &value,
			Value &const_value_out);
		InternalExceptionPointer analyze_arithmetic_ins(
			ProgramAnalyzeContext &analyze_context,
			uint32_t reg_index) noexcept;
		InternalExceptionPointer analyze_cast_ins(
			ProgramAnalyzeContext &analyze_context,
			uint32_t reg_index);
		InternalExceptionPointer analyze_program_info_pass(
			Runtime *runtime,
			peff::Alloc *resource_allocator,
			RegularFnOverloadingObject *fn_object,
			ProgramAnalyzedInfo &analyzed_info_out,
			HostRefHolder &host_ref_holder);
	}
}

#endif
