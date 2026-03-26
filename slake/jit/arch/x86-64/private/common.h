#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMMON_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMMON_H_

#include "emitters.h"
#include "../context.h"
#include <peff/containers/map.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			struct JITRelAddrReplacingPoint32 {
				uint8_t offset;
				const void *dest;
			};

			struct JITRelAddrReplacingPoint32Storage {
				uint8_t num_replacing_points = 0;
				JITRelAddrReplacingPoint32 replacing_points[3];

				SLAKE_FORCEINLINE void push_replacing_point(
					const JITRelAddrReplacingPoint32 &replacing_point) {
					assert(num_replacing_points < std::size(replacing_points));
					replacing_points[num_replacing_points++] = replacing_point;
				}
			};

			struct PhysicalRegSavingInfo {
				uint32_t vreg_id;
			};

			struct PhysicalRegState {
				uint32_t last_vreg_id;
				peff::Uninit<peff::List<PhysicalRegSavingInfo>> saving_info;
			};

			struct VirtualRegState {
				RegisterId phy_reg;
				int32_t save_offset = INT32_MIN;
				size_t size;
			};

			struct CallingRegSavingInfo {
				int32_t off_saved_rax = INT32_MIN,
						off_saved_r10 = INT32_MIN,
						off_saved_r11 = INT32_MIN;
				size_t sz_saved_rax,
					sz_saved_r10,
					sz_saved_r11;
				int32_t off_saved_rcx, off_saved_rdx, off_saved_r8, off_saved_r9;
				size_t sz_saved_rcx, sz_saved_rdx, sz_saved_r8, sz_saved_r9;
			};

			struct LocalVarState {
				Type type;
				int32_t stack_off;
				size_t size;
			};

			struct JITCompileContext {
				Runtime *runtime;
				peff::RcObjectPtr<peff::Alloc> resource_allocator;
				peff::List<DiscreteInstruction> native_instructions;
				PhysicalRegState phy_reg_states[REG_MAX];
				size_t cur_stack_size;
				JITCompilerOptions options;
				std::bitset<REG_MAX> reg_alloc_flags;
				peff::Map<uint32_t, VirtualRegState> virtual_reg_states;
				peff::Map<uint32_t, LocalVarState> local_var_states;
				peff::Map<size_t, peff::List<uint32_t>> reg_recycle_boundaries;
				peff::Map<int32_t, size_t> free_stack_spaces;
				int32_t jit_context_off;
				peff::HashMap<peff::String, size_t> label_offsets;

				SLAKE_API JITCompileContext(Runtime *runtime, peff::Alloc *resource_allocator);
				[[nodiscard]] SLAKE_API InternalExceptionPointer push_prolog_stack_op_ins();
				[[nodiscard]] SLAKE_API InternalExceptionPointer push_epilog_stack_op_ins();

				[[nodiscard]] SLAKE_API InternalExceptionPointer check_stack_pointer(uint32_t size);
				[[nodiscard]] SLAKE_API InternalExceptionPointer check_stack_pointer_on_prolog(uint32_t size);
				[[nodiscard]] SLAKE_API InternalExceptionPointer check_and_push_stack_pointer(uint32_t size);
				[[nodiscard]] SLAKE_API InternalExceptionPointer check_and_push_stack_pointer_on_prolog(uint32_t size);

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer init_jitcontext_storage() noexcept {
					SLAKE_RETURN_IF_EXCEPT(stack_alloc_aligned(sizeof(JITExecContext *), sizeof(JITExecContext *), jit_context_off));

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_ins(DiscreteInstruction &&ins) noexcept {
					if (!native_instructions.push_back(std::move(ins)))
						return OutOfMemoryError::alloc();
					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_label(peff::String &&label) noexcept {
					if (!label_offsets.insert(std::move(label), native_instructions.size()))
						return OutOfMemoryError::alloc();
					return {};
				}

				SLAKE_FORCEINLINE void add_stack_ptr(size_t size) noexcept {
					cur_stack_size += size;
				}

				SLAKE_FORCEINLINE void sub_stack_ptr(size_t size) noexcept {
					cur_stack_size -= size;
				}

				SLAKE_API RegisterId alloc_gp_reg() noexcept;
				SLAKE_API RegisterId alloc_xmm_reg() noexcept;

				SLAKE_API void set_reg_allocated(RegisterId reg);
				SLAKE_API void unalloc_reg(RegisterId reg);

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_reg8(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
					int32_t lhs_reg_stack_save_off;
					SLAKE_RETURN_IF_EXCEPT(stack_alloc_aligned(sizeof(uint8_t), sizeof(uint8_t), lhs_reg_stack_save_off));

					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_reg8_to_mem_ins(MemoryLocation{ REG_RBP, lhs_reg_stack_save_off, REG_MAX, 0 }, reg)));

					off_out = lhs_reg_stack_save_off;
					size_out = sizeof(uint8_t);

					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = lhs_reg_stack_save_off;
					if (!phy_reg_states[reg].saving_info->push_back({ phy_reg_states[reg].last_vreg_id }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_reg16(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
					int32_t lhs_reg_stack_save_off;
					SLAKE_RETURN_IF_EXCEPT(stack_alloc_aligned(sizeof(uint16_t), sizeof(uint16_t), lhs_reg_stack_save_off));

					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_reg8_to_mem_ins(MemoryLocation{ REG_RBP, lhs_reg_stack_save_off, REG_MAX, 0 }, reg)));

					off_out = lhs_reg_stack_save_off;
					size_out = sizeof(uint16_t);

					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = lhs_reg_stack_save_off;
					if (!phy_reg_states[reg].saving_info->push_back({ phy_reg_states[reg].last_vreg_id }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_reg32(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
					int32_t lhs_reg_stack_save_off;
					SLAKE_RETURN_IF_EXCEPT(stack_alloc_aligned(sizeof(uint32_t), sizeof(uint32_t), lhs_reg_stack_save_off));

					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_reg8_to_mem_ins(MemoryLocation{ REG_RBP, lhs_reg_stack_save_off, REG_MAX, 0 }, reg)));

					off_out = lhs_reg_stack_save_off;
					size_out = sizeof(uint32_t);

					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = lhs_reg_stack_save_off;
					if (!phy_reg_states[reg].saving_info->push_back({ phy_reg_states[reg].last_vreg_id }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_reg64(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
					int32_t lhs_reg_stack_save_off;
					SLAKE_RETURN_IF_EXCEPT(stack_alloc_aligned(sizeof(uint64_t), sizeof(uint64_t), lhs_reg_stack_save_off));

					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_reg8_to_mem_ins(MemoryLocation{ REG_RBP, lhs_reg_stack_save_off, REG_MAX, 0 }, reg)));

					off_out = lhs_reg_stack_save_off;
					size_out = sizeof(uint64_t);

					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = lhs_reg_stack_save_off;
					if (!phy_reg_states[reg].saving_info->push_back({ phy_reg_states[reg].last_vreg_id }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer push_reg(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept;

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_reg_xmm32(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
					int32_t tmp_off;
					size_t tmp_size;
					RegisterId gp_reg = alloc_gp_reg();

					SLAKE_RETURN_IF_EXCEPT(push_reg(gp_reg, tmp_off, tmp_size));

					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_movd_reg_xmm_to_reg32_ins(gp_reg, reg)));
					SLAKE_RETURN_IF_EXCEPT(push_reg32(gp_reg, off_out, size_out));

					SLAKE_RETURN_IF_EXCEPT(pop_reg(gp_reg, tmp_off, tmp_size));

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer push_reg_xmm64(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
					int32_t tmp_off;
					size_t tmp_size;
					RegisterId gp_reg = alloc_gp_reg();

					SLAKE_RETURN_IF_EXCEPT(push_reg(gp_reg, tmp_off, tmp_size));

					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_movq_reg_xmm_to_reg64_ins(gp_reg, reg)));
					SLAKE_RETURN_IF_EXCEPT(push_reg64(gp_reg, off_out, size_out));

					SLAKE_RETURN_IF_EXCEPT(pop_reg(gp_reg, tmp_off, tmp_size));

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer push_reg_xmm(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept;

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pop_reg8(RegisterId reg, int32_t off) noexcept {
					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_mem_to_reg8_ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stack_free(off, sizeof(uint8_t));

					virtual_reg_states.at(phy_reg_states[reg].saving_info->back().vreg_id).save_offset = INT32_MIN;
					phy_reg_states[reg].saving_info->pop_back();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pop_reg16(RegisterId reg, int32_t off) noexcept {
					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_mem_to_reg16_ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stack_free(off, sizeof(uint16_t));

					virtual_reg_states.at(phy_reg_states[reg].saving_info->back().vreg_id).save_offset = INT32_MIN;
					phy_reg_states[reg].saving_info->pop_back();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pop_reg32(RegisterId reg, int32_t off) noexcept {
					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_mem_to_reg32_ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stack_free(off, sizeof(uint32_t));

					virtual_reg_states.at(phy_reg_states[reg].saving_info->back().vreg_id).save_offset = INT32_MIN;
					phy_reg_states[reg].saving_info->pop_back();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pop_reg64(RegisterId reg, int32_t off) noexcept {
					virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).save_offset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_mem_to_reg64_ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stack_free(off, sizeof(uint64_t));

					virtual_reg_states.at(phy_reg_states[reg].saving_info->back().vreg_id).save_offset = INT32_MIN;
					phy_reg_states[reg].saving_info->pop_back();

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer pop_reg(RegisterId reg, int32_t off, size_t size) noexcept;

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pop_reg_xmm32(RegisterId reg, int32_t off) noexcept {
					int32_t tmp_off;
					size_t tmp_size;
					RegisterId gp_reg = alloc_gp_reg();

					SLAKE_RETURN_IF_EXCEPT(push_reg(gp_reg, tmp_off, tmp_size));

					SLAKE_RETURN_IF_EXCEPT(pop_reg32(gp_reg, off));
					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_movd_reg32_to_reg_xmm_ins(gp_reg, reg)));

					SLAKE_RETURN_IF_EXCEPT(pop_reg(gp_reg, tmp_off, tmp_size));

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pop_reg_xmm64(RegisterId reg, int32_t off) noexcept {
					int32_t tmp_off;
					size_t tmp_size;
					RegisterId gp_reg = alloc_gp_reg();

					SLAKE_RETURN_IF_EXCEPT(push_reg(gp_reg, tmp_off, tmp_size));

					SLAKE_RETURN_IF_EXCEPT(pop_reg64(gp_reg, off));
					SLAKE_RETURN_IF_EXCEPT(push_ins(emit_movq_reg64_to_reg_xmm_ins(gp_reg, reg)));

					SLAKE_RETURN_IF_EXCEPT(pop_reg(gp_reg, tmp_off, tmp_size));

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer pop_reg_xmm(RegisterId reg, int32_t off, size_t size) noexcept;

				SLAKE_FORCEINLINE bool is_reg_in_use(RegisterId reg) noexcept {
					return phy_reg_states[reg].last_vreg_id == UINT32_MAX;
				}

				[[nodiscard]] SLAKE_FORCEINLINE VirtualRegState *def_virtual_reg(uint32_t vreg, RegisterId phy_reg, size_t size) noexcept {
					if (!virtual_reg_states.insert(+vreg, {}))
						return nullptr;
					VirtualRegState &vreg_state = virtual_reg_states.at(vreg);
					vreg_state.phy_reg = phy_reg;
					vreg_state.size = size;
					vreg_state.save_offset = INT32_MIN;

					phy_reg_states[phy_reg].last_vreg_id = vreg;

					return &vreg_state;
				}
				[[nodiscard]] SLAKE_FORCEINLINE VirtualRegState *def_virtual_reg(uint32_t vreg, int32_t save_offset, size_t size) noexcept {
					if (!virtual_reg_states.insert(+vreg, {}))
						return nullptr;
					VirtualRegState &vreg_state = virtual_reg_states.at(vreg);
					vreg_state.phy_reg = REG_MAX;
					vreg_state.size = size;
					vreg_state.save_offset = save_offset;

					return &vreg_state;
				}
				[[nodiscard]] SLAKE_FORCEINLINE VirtualRegState *def_dummy_virtual_reg(uint32_t vreg) noexcept {
					if (!virtual_reg_states.insert(+vreg, {}))
						return nullptr;
					VirtualRegState &vreg_state = virtual_reg_states.at(vreg);
					vreg_state.phy_reg = REG_MAX;
					vreg_state.size = 0;
					vreg_state.save_offset = 0;

					return &vreg_state;
				}

				[[nodiscard]] SLAKE_FORCEINLINE LocalVarState *def_local_var(uint32_t index, int32_t stack_off, size_t size) noexcept {
					if (!local_var_states.insert(+index, {}))
						return nullptr;
					LocalVarState &local_var_state = local_var_states.at(index);
					local_var_state.stack_off = stack_off;
					local_var_state.size = size;

					return &local_var_state;
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer stack_alloc_aligned(uint32_t size, uint32_t alignment, int32_t &off_out) noexcept;

				SLAKE_API void stack_free(int32_t save_offset, size_t size);

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer save_calling_regs(CallingRegSavingInfo &info_out) noexcept {
					// Save parameter registers.
					SLAKE_RETURN_IF_EXCEPT(push_reg(REG_RCX, info_out.off_saved_rcx, info_out.sz_saved_rcx));
					SLAKE_RETURN_IF_EXCEPT(push_reg(REG_RDX, info_out.off_saved_rdx, info_out.sz_saved_rdx));
					SLAKE_RETURN_IF_EXCEPT(push_reg(REG_R8, info_out.off_saved_r8, info_out.sz_saved_r8));
					SLAKE_RETURN_IF_EXCEPT(push_reg(REG_R9, info_out.off_saved_r9, info_out.sz_saved_r9));
					// Save scratch registers.
					if (is_reg_in_use(REG_RAX)) {
						SLAKE_RETURN_IF_EXCEPT(push_reg(REG_RAX, info_out.off_saved_rax, info_out.sz_saved_rax));
					}
					if (is_reg_in_use(REG_R10)) {
						SLAKE_RETURN_IF_EXCEPT(push_reg(REG_R10, info_out.off_saved_r10, info_out.sz_saved_r10));
					}
					if (is_reg_in_use(REG_R11)) {
						SLAKE_RETURN_IF_EXCEPT(push_reg(REG_R11, info_out.off_saved_r11, info_out.sz_saved_r11));
					}
					return {};
				}

				SLAKE_FORCEINLINE InternalExceptionPointer restore_calling_regs(const CallingRegSavingInfo &info) noexcept {
					// Restore scratch registers.
					if (info.off_saved_r11 != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_R11, info.off_saved_r11, info.sz_saved_r11));
					}
					if (info.off_saved_r10 != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_R10, info.off_saved_r10, info.sz_saved_r10));
					}
					if (info.off_saved_rax != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_RAX, info.off_saved_rax, info.sz_saved_rax));
					}

					// Restore parameter registers.
					SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_R9, info.off_saved_r9, info.sz_saved_r9));
					SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_R8, info.off_saved_r8, info.sz_saved_r8));
					SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_RDX, info.off_saved_rdx, info.sz_saved_rdx));
					SLAKE_RETURN_IF_EXCEPT(pop_reg(REG_RCX, info.off_saved_rcx, info.sz_saved_rcx));

					return {};
				}
			};

			struct JITExecContext;

			typedef void (*JITCompiledFnPtr)(JITExecContext *exec_context);

			void load_ins_wrapper(
				JITExecContext *context,
				IdRefObject *id_ref_object);
			void rload_ins_wrapper(
				JITExecContext *context,
				MemberObject *base_object,
				IdRefObject *id_ref_object);
			void memcpy_wrapper(
				void *dest,
				const void *src,
				uint64_t size);
		}
	}
}

#endif
