#include "common.h"
#include "../context.h"
#include <slake/runtime.h>

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

static const RegisterId _s_gp_regs[] = {
	REG_RAX,
	REG_RBX,
	REG_RCX,
	REG_RDX,
	REG_RSI,
	REG_RDI,
	REG_R8,
	REG_R9,
	REG_R10,
	// R11 will never be allocated since it is used for stack limit checking.
	REG_R12,
	REG_R13,
	REG_R14,
	REG_R15
};

static const RegisterId _s_xmm_regs[] = {
	REG_XMM0,
	REG_XMM1,
	REG_XMM2,
	REG_XMM3,
	REG_XMM4,
	REG_XMM5,
	REG_XMM6,
	REG_XMM7,
	REG_XMM8,
	REG_XMM9,
	REG_XMM10,
	REG_XMM11,
	REG_XMM12,
	REG_XMM13,
	REG_XMM14,
	REG_XMM15,
};

SLAKE_API JITCompileContext::JITCompileContext(Runtime *runtime, peff::Alloc *resource_allocator)
	: native_instructions(resource_allocator),
	  virtual_reg_states(resource_allocator),
	  local_var_states(resource_allocator),
	  reg_recycle_boundaries(resource_allocator),
	  free_stack_spaces(resource_allocator),
	  label_offsets(resource_allocator) {
	for (size_t i = 0; i < REG_MAX; ++i) {
		phy_reg_states[i].saving_info.move_from(peff::List<PhysicalRegSavingInfo>(resource_allocator));
	}
}

SLAKE_API InternalExceptionPointer JITCompileContext::push_prolog_stack_op_ins() {
	SLAKE_RETURN_IF_EXCEPT(check_stack_pointer_on_prolog(sizeof(uint64_t) * 11));

	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_R8)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_RDX)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_RCX)));

	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_RBX)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_RDI)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_RSI)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_R12)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_R13)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_R14)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_R15)));

	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_push_reg64_ins(REG_RBP)));

	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_reg64_to_reg64_ins(REG_RBP, REG_RSP)));

	add_stack_ptr(sizeof(uint64_t) * 11);

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::push_epilog_stack_op_ins() {
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_mov_reg64_to_reg64_ins(REG_RSP, REG_RBP)));

	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_RBP)));

	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_R15)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_R14)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_R13)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_R12)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_RSI)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_RDI)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_RBX)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_RCX)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_RDX)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_pop_reg64_ins(REG_R8)));

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::check_stack_pointer(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_sub_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_cmp_reg64_to_reg64_ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_labelled_jump_ins("_report_stack_overflow", DiscreteInstructionType::JumpIfLtLabelled)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_add_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&size)));

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::check_stack_pointer_on_prolog(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_sub_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_cmp_reg64_to_reg64_ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_labelled_jump_ins("_report_stack_overflow_on_prolog", DiscreteInstructionType::JumpIfLtLabelled)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_add_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&size)));

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::check_and_push_stack_pointer(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_sub_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_cmp_reg64_to_reg64_ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_labelled_jump_ins("_report_stack_overflow", DiscreteInstructionType::JumpIfLtLabelled)));
	add_stack_ptr(size);

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::check_and_push_stack_pointer_on_prolog(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_sub_imm32_to_reg64_ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_cmp_reg64_to_reg64_ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(push_ins(emit_labelled_jump_ins("_report_stack_overflow_on_prolog", DiscreteInstructionType::JumpIfLtLabelled)));
	add_stack_ptr(size);

	return {};
}

SLAKE_API RegisterId JITCompileContext::alloc_gp_reg() noexcept {
realloc:
	for (auto i : _s_gp_regs) {
		if (!reg_alloc_flags.test(i)) {
			reg_alloc_flags.set(i);
			return i;
		}
	}
	for (auto i : _s_gp_regs) {
		reg_alloc_flags.reset(i);
	}
	goto realloc;
}

SLAKE_API RegisterId JITCompileContext::alloc_xmm_reg() noexcept {
realloc:
	for (auto i : _s_xmm_regs) {
		if (!reg_alloc_flags.test(i)) {
			reg_alloc_flags.set(i);
			return i;
		}
	}
	for (auto i : _s_xmm_regs) {
		reg_alloc_flags.reset(i);
	}
	goto realloc;
}

SLAKE_API void JITCompileContext::set_reg_allocated(RegisterId reg) {
	reg_alloc_flags.set(reg);
}

SLAKE_API void JITCompileContext::unalloc_reg(RegisterId reg) {
	assert(reg_alloc_flags.test(reg));
	reg_alloc_flags.reset(reg);
}

SLAKE_API InternalExceptionPointer JITCompileContext::push_reg(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
	switch (virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).size) {
		case sizeof(uint8_t):
			SLAKE_RETURN_IF_EXCEPT(push_reg8(reg, off_out, size_out));
			break;
		case sizeof(uint16_t):
			SLAKE_RETURN_IF_EXCEPT(push_reg16(reg, off_out, size_out));
			break;
		case sizeof(uint32_t):
			SLAKE_RETURN_IF_EXCEPT(push_reg32(reg, off_out, size_out));
			break;
		case sizeof(uint64_t):
			SLAKE_RETURN_IF_EXCEPT(push_reg64(reg, off_out, size_out));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::push_reg_xmm(RegisterId reg, int32_t &off_out, size_t &size_out) noexcept {
	switch (virtual_reg_states.at(phy_reg_states[reg].last_vreg_id).size) {
		case sizeof(float):
			SLAKE_RETURN_IF_EXCEPT(push_reg_xmm32(reg, off_out, size_out));
			break;
		case sizeof(double):
			SLAKE_RETURN_IF_EXCEPT(push_reg_xmm64(reg, off_out, size_out));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::pop_reg(RegisterId reg, int32_t off, size_t size) noexcept {
	switch (size) {
		case sizeof(uint8_t):
			SLAKE_RETURN_IF_EXCEPT(pop_reg8(reg, off));
			break;
		case sizeof(uint16_t):
			SLAKE_RETURN_IF_EXCEPT(pop_reg16(reg, off));
			break;
		case sizeof(uint32_t):
			SLAKE_RETURN_IF_EXCEPT(pop_reg32(reg, off));
			break;
		case sizeof(uint64_t):
			SLAKE_RETURN_IF_EXCEPT(pop_reg64(reg, off));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::pop_reg_xmm(RegisterId reg, int32_t off, size_t size) noexcept {
	switch (size) {
		case sizeof(float):
			SLAKE_RETURN_IF_EXCEPT(pop_reg_xmm32(reg, off));
			break;
		case sizeof(double):
			SLAKE_RETURN_IF_EXCEPT(pop_reg_xmm64(reg, off));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::stack_alloc_aligned(uint32_t size, uint32_t alignment, int32_t &off_out) noexcept {
	for (auto i = free_stack_spaces.begin(); i != free_stack_spaces.end(); ++i) {
		int32_t alloc_base = i.key();
		size_t diff = alloc_base % alignment;
		if (diff) {
			alloc_base += alignment - diff;
		}

		if (i.value() > size) {
			int32_t new_base = (int32_t)i.key() + size;

			// We don't merge here, just shrink the space.
			free_stack_spaces.insert(std::move(new_base), i.value() - size);
			free_stack_spaces.remove(i.key());

			off_out = (int32_t)-alloc_base;
			return {};
		} else if (i.value() == size) {
			int32_t off = i.key();
			free_stack_spaces.remove(i.key());
			off_out = -off;
			return {};
		}
	}

	int32_t alloc_base = (int32_t)cur_stack_size;
	size_t diff = cur_stack_size % alignment;
	if (diff) {
		alloc_base += alignment - diff;
	}
	size += alignment - diff;

	SLAKE_RETURN_IF_EXCEPT(check_and_push_stack_pointer(size));

	off_out = -alloc_base;
	return {};
}

SLAKE_API void JITCompileContext::stack_free(int32_t save_offset, size_t size) {
	free_stack_spaces.insert(+save_offset, +size);
	auto self_it = free_stack_spaces.find(save_offset);

	// Try to merge with the right neighbor.
	{
		auto right_neighbor = self_it;
		++right_neighbor;

		if (right_neighbor != free_stack_spaces.end()) {
			assert(right_neighbor.value() >= save_offset + sizeof(uint64_t));
			if (right_neighbor.key() == save_offset + sizeof(uint64_t)) {
				self_it.value() += right_neighbor.value();
				free_stack_spaces.remove(right_neighbor);
			}
		}
	}

	// Try to merge the new slot with the left neighbor.
	if (self_it != free_stack_spaces.begin()) {
		auto left_neighbor = self_it;
		--left_neighbor;

		assert(left_neighbor.key() + left_neighbor.value() <= save_offset);
		if (left_neighbor.key() + left_neighbor.value() == save_offset) {
			left_neighbor.value() += self_it.value();
			free_stack_spaces.remove(self_it);
		}
	}
}

void slake::jit::x86_64::load_ins_wrapper(
	JITExecContext *context,
	IdRefObject *id_ref_object) {
	Reference entity_ref;
	InternalExceptionPointer e = context->runtime->resolve_id_ref(id_ref_object, entity_ref, nullptr);
	if (e) {
		context->exception = e.get();
		e.reset();
		return;
	}
	context->return_value = Value(entity_ref);
}
void slake::jit::x86_64::rload_ins_wrapper(
	JITExecContext *context,
	MemberObject *base_object,
	IdRefObject *id_ref_object) {
	Reference entity_ref;
	InternalExceptionPointer e = context->runtime->resolve_id_ref(id_ref_object, entity_ref, base_object);
	if (e) {
		context->exception = e.get();
		e.reset();
		return;
	}
	context->return_value = Value(entity_ref);
}

void slake::jit::x86_64::memcpy_wrapper(
	void *dest,
	const void *src,
	uint64_t size) {
	memmove(dest, src, size);
}
