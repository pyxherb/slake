#include <cstdint>
#include <cstring>

#include "common.h"
#include <slake/opti/proganal.h>

#include "emitters.h"
#include "comp/add.h"
#include "comp/sub.h"
#include "comp/mul.h"
#include "comp/div.h"
#include "comp/mod.h"
#include "comp/mov.h"
#include "comp/store.h"
#include "comp/bitshift.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

InternalExceptionPointer compile_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins) {
	uint32_t output_reg_index = UINT32_MAX;
	if (cur_ins.output != UINT32_MAX) {
		output_reg_index = cur_ins.output;
		size_t off_timeline_end = analyzed_info.analyzed_reg_info.at(output_reg_index).lifetime.off_end_ins;

		if (!compile_context.reg_recycle_boundaries.contains(off_timeline_end)) {
			compile_context.reg_recycle_boundaries.insert(+off_timeline_end, peff::List<uint32_t>(compile_context.resource_allocator.get()));
		}

		if (!compile_context.reg_recycle_boundaries.at(off_timeline_end).push_back(+output_reg_index))
			return OutOfMemoryError::alloc();
	}

	if (auto it = compile_context.reg_recycle_boundaries.find(off_ins);
		it != compile_context.reg_recycle_boundaries.end()) {
		for (auto i : it.value()) {
			VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(i);

			if (vreg_state.save_offset != INT32_MIN) {
				compile_context.stack_free(vreg_state.save_offset, vreg_state.size);
			} else {
				compile_context.reg_alloc_flags.reset(vreg_state.phy_reg);
			}
		}
		compile_context.reg_recycle_boundaries.remove(it);
	}

	switch (cur_ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LOAD: {
			uint32_t output_reg_index = cur_ins.output;

			/*
			{
				Value expected_value = analyzed_info.analyzed_reg_info.at(output_reg_index).expected_value;

				if (expected_value.value_type != ValueType::Undefined) {
					Instruction ins = { Opcode::COPY, cur_ins.output, { expected_value } };
					compile_instruction(compile_context, analyzed_info, SIZE_MAX, ins);
					return {};
				}
			}*/

			CallingRegSavingInfo calling_reg_saving_info;

			SLAKE_RETURN_IF_EXCEPT(compile_context.save_calling_regs(calling_reg_saving_info));

			// Pass the first argument.
			{
				SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_RCX, MemoryLocation{ REG_RBP, compile_context.jit_context_off, REG_MAX, 0 })));
			}

			// Pass the second argument.
			{
				IdRefObject *ref_obj = (IdRefObject *)cur_ins.operands[0].get_reference().as_object;

				SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_imm64_to_reg64_ins(REG_RDX, (uint8_t *)&ref_obj)));
			}

			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_call_ins((void *)load_ins_wrapper)));

			// Psas the first argument for the memcpy wrapper.
			int32_t stack_off;
			SLAKE_RETURN_IF_EXCEPT(compile_context.stack_alloc_aligned(sizeof(Value), sizeof(Value), stack_off));
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_reg64_to_reg64_ins(REG_RCX, REG_RBP)));
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_add_imm32_to_reg64_ins(REG_RCX, (uint8_t *)&stack_off)));

			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_RDX, MemoryLocation{ REG_RBP, compile_context.jit_context_off, REG_MAX, 0 })));

			// Psas the second argument for the memcpy wrapper.
			static int32_t return_value_off = -(int32_t)offsetof(JITExecContext, return_value);
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_add_imm32_to_reg64_ins(REG_RDX, (uint8_t *)&return_value_off)));

			// Psas the third argument for the memcpy wrapper.
			static uint64_t size = sizeof(Value);
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_imm64_to_reg64_ins(REG_R8, (uint8_t *)&size)));

			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_call_ins((void *)memcpy_wrapper)));

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, stack_off, sizeof(Value));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			SLAKE_RETURN_IF_EXCEPT(compile_context.restore_calling_regs(calling_reg_saving_info));

			break;
		}
		case Opcode::RLOAD: {
			uint32_t output_reg_index = cur_ins.output,
					 base_object_reg_index = cur_ins.operands[0].get_reg_index();

			/*
			{
				Value expected_value = analyzed_info.analyzed_reg_info.at(output_reg_index).expected_value;

				if (expected_value.value_type != ValueType::Undefined) {
					Instruction ins = { Opcode::COPY, cur_ins.output, { expected_value } };
					compile_instruction(compile_context, analyzed_info, SIZE_MAX, ins);
					return {};
				}
			}*/

			CallingRegSavingInfo calling_reg_saving_info;

			SLAKE_RETURN_IF_EXCEPT(compile_context.save_calling_regs(calling_reg_saving_info));

			// Pass the first argument.
			{
				SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_RCX, MemoryLocation{ REG_RBP, compile_context.jit_context_off, REG_MAX, 0 })));
			}

			// Pass the second argument.
			{
				auto &vreg_state = compile_context.virtual_reg_states.at(base_object_reg_index);
				if (vreg_state.save_offset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(
						REG_RDX,
						MemoryLocation{
							REG_RBP,
							compile_context.virtual_reg_states.at(base_object_reg_index).save_offset,
							REG_MAX,
							0 })));
				} else {
					if (vreg_state.phy_reg != REG_RDX) {
						SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_reg64_to_reg64_ins(vreg_state.phy_reg, REG_RDX)));
					}
				}
			}

			// Pass the third argument.
			{
				IdRefObject *ref_obj = (IdRefObject *)cur_ins.operands[1].get_reference().as_object;

				SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_imm64_to_reg64_ins(REG_R8, (uint8_t *)&ref_obj)));
			}

			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_call_ins((void *)rload_ins_wrapper)));

			// Psas the first argument for the memcpy wrapper.
			int32_t stack_off;
			SLAKE_RETURN_IF_EXCEPT(compile_context.stack_alloc_aligned(sizeof(Value), sizeof(Value), stack_off));
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_reg64_to_reg64_ins(REG_RCX, REG_RBP)));
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_add_imm32_to_reg64_ins(REG_RCX, (uint8_t *)&stack_off)));

			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_RDX, MemoryLocation{ REG_RBP, compile_context.jit_context_off, REG_MAX, 0 })));

			// Psas the second argument for the memcpy wrapper.
			static int32_t return_value_off = -(int32_t)offsetof(JITExecContext, return_value);
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_add_imm32_to_reg64_ins(REG_RDX, (uint8_t *)&return_value_off)));

			// Psas the third argument for the memcpy wrapper.
			static uint64_t size = sizeof(Value);
			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_imm64_to_reg64_ins(REG_R8, (uint8_t *)&size)));

			SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_call_ins((void *)memcpy_wrapper)));

			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, stack_off, sizeof(Value));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();

			SLAKE_RETURN_IF_EXCEPT(compile_context.restore_calling_regs(calling_reg_saving_info));

			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT(compile_store_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::LARG: {
			break;
		}
		case Opcode::LVALUE: {
		}
		case Opcode::COPY: {
			SLAKE_RETURN_IF_EXCEPT(compile_mov_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT(compile_add_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT(compile_sub_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT(compile_mul_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT(compile_div_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT(compile_div_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::LSH: {
			SLAKE_RETURN_IF_EXCEPT(compile_shl_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT(compile_shr_instruction(compile_context, analyzed_info, off_ins, cur_ins));
			break;
		}
	}

	return {};
}

InternalExceptionPointer slake::compile_regular_fn(RegularFnOverloadingObject *fn, peff::Alloc *resource_allocator, const JITCompilerOptions &options) {
	slake::CodePage *code_page;
	size_t size;

	JITCompileContext compile_context(fn->associated_runtime, resource_allocator);
	size_t num_ins = fn->instructions.size();

	opti::ProgramAnalyzedInfo analyzed_info(fn->associated_runtime, resource_allocator);
	HostRefHolder host_ref_holder(resource_allocator);

	InternalExceptionPointer exception_ptr;

	SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception_ptr, opti::analyze_program_info_pass(fn->associated_runtime, resource_allocator, fn, analyzed_info, host_ref_holder));

	SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_R11, MemoryLocation{ REG_R9, offsetof(JITExecContext, stack_limit), REG_MAX, 0 })));

	SLAKE_RETURN_IF_EXCEPT(compile_context.push_prolog_stack_op_ins());

	// R11 is used for stack limit checking.
	compile_context.set_reg_allocated(REG_R11);

	{
		SLAKE_RETURN_IF_EXCEPT(compile_context.init_jitcontext_storage());
		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(
			emit_mov_reg64_to_mem_ins(
				MemoryLocation{
					REG_RBP, compile_context.jit_context_off,
					REG_MAX, 0 },
				REG_R9)));

		for (size_t i = 0; i < num_ins; ++i) {
			const Instruction &cur_ins = fn->instructions.at(i);

			SLAKE_RETURN_IF_EXCEPT(compile_instruction(compile_context, analyzed_info, i, cur_ins));
		}

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_epilog_stack_op_ins());

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_near_ret_ins()));
	}

	{
		peff::String label_name(resource_allocator);
		if (!label_name.build("_report_stack_overflow"))
			return OutOfMemoryError::alloc();
		SLAKE_RETURN_IF_EXCEPT(compile_context.push_label(std::move(label_name)));

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_epilog_stack_op_ins());

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_R11, MemoryLocation{ REG_R9, offsetof(JITExecContext, stack_overflow_error), REG_MAX, 0 })));
		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_reg64_to_mem_ins(MemoryLocation{ REG_R9, offsetof(JITExecContext, exception), REG_MAX, 0 }, REG_R11)));

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_near_ret_ins()));
	}

	{
		peff::String label_name(resource_allocator);
		if (!label_name.build("_report_stack_overflow_on_prolog"))
			return OutOfMemoryError::alloc();
		SLAKE_RETURN_IF_EXCEPT(compile_context.push_label(std::move(label_name)));

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_mem_to_reg64_ins(REG_R11, MemoryLocation{ REG_R9, offsetof(JITExecContext, stack_overflow_error), REG_MAX, 0 })));
		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_mov_reg64_to_mem_ins(MemoryLocation{ REG_R9, offsetof(JITExecContext, exception), REG_MAX, 0 }, REG_R11)));

		SLAKE_RETURN_IF_EXCEPT(compile_context.push_ins(emit_near_ret_ins()));
	}

	return {};
}
