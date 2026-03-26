#include "../cfg.h"

using namespace slake;
using namespace slake::opti;

SLAKE_API InternalExceptionPointer slake::opti::divide_instructions_into_basic_blocks(peff::Alloc *intermediate_allocator, RegularFnOverloadingObject *fn_overloading, peff::Alloc *output_allocator, ControlFlowGraph &control_flow_graph_out) {
	Runtime *const rt = fn_overloading->associated_runtime;
	const RegularFnOverloadingObject *ol = const_cast<const RegularFnOverloadingObject *>(fn_overloading);
	peff::Set<size_t> basic_block_boundaries(intermediate_allocator);	 // Basic block boundaries
	peff::Map<size_t, size_t> basic_block_map(intermediate_allocator);	 // Basic block offset-to-label-id map

	for (size_t i = 0; i < ol->instructions.size(); ++i) {
		const Instruction &ins = ol->instructions.at(i);

		switch (ins.opcode) {
			case Opcode::JMP: {
				if (ins.num_operands != 1)
					return MalformedProgramError::alloc(rt->get_fixed_alloc(), fn_overloading, i);
				if (ins.operands[0].value_type != ValueType::U32)
					return MalformedProgramError::alloc(rt->get_fixed_alloc(), fn_overloading, i);
				if (!basic_block_boundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!basic_block_boundaries.insert(ins.operands[0].get_u32()))
					return OutOfMemoryError::alloc();
				break;
			}
			case Opcode::BR: {
				if (ins.num_operands != 3)
					return MalformedProgramError::alloc(rt->get_fixed_alloc(), fn_overloading, i);
				if (ins.operands[1].value_type != ValueType::U32)
					return MalformedProgramError::alloc(rt->get_fixed_alloc(), fn_overloading, i);
				if (ins.operands[2].value_type != ValueType::U32)
					return MalformedProgramError::alloc(rt->get_fixed_alloc(), fn_overloading, i);
				if (!basic_block_boundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!basic_block_boundaries.insert(ins.operands[1].get_u32()))
					return OutOfMemoryError::alloc();
				if (!basic_block_boundaries.insert(ins.operands[2].get_u32()))
					return OutOfMemoryError::alloc();
				break;
			}
			case Opcode::RET: {
				if (ins.num_operands > 1)
					return MalformedProgramError::alloc(rt->get_fixed_alloc(), fn_overloading, i);
				if (i + 1 < ol->instructions.size())
					if (!basic_block_boundaries.insert(i + 1))
						return OutOfMemoryError::alloc();
				break;
			}
			default:
				break;
		}
	}

	if (!basic_block_map.insert(0, 0))
		return OutOfMemoryError::alloc();
	{
		auto it = basic_block_boundaries.begin();
		for (size_t i = 0; i <= basic_block_boundaries.size(); ++i) {
			size_t block_begin;
			if (it == basic_block_boundaries.begin())
				block_begin = 0;
			else
				block_begin = *it.prev();
			if (!basic_block_map.insert(+block_begin, basic_block_map.size()))
				return OutOfMemoryError::alloc();
			if (i < basic_block_boundaries.size())
				++it;
		}
	}

	if (!control_flow_graph_out.basic_blocks.resize_uninit(basic_block_boundaries.size() + 1))
		return OutOfMemoryError::alloc();
	for (size_t i = 0; i < control_flow_graph_out.basic_blocks.size(); ++i)
		peff::construct_at<BasicBlock>(&control_flow_graph_out.basic_blocks.at(i), output_allocator);

	{
		auto it = basic_block_boundaries.begin();
		for (size_t i = 0; i <= basic_block_boundaries.size(); ++i) {
			const size_t block_begin = i ? *it.prev() : 0,
						 block_end = i < basic_block_boundaries.size() ? *it++ : ol->instructions.size();

			BasicBlock cur_block(output_allocator);

			if (!cur_block.instructions.resize(block_end - block_begin))
				return OutOfMemoryError::alloc();

			for (size_t j = block_begin, k = 0; j < block_end; ++j, ++k) {
				const Instruction &original_instruction = fn_overloading->instructions.at(j);

				Instruction &new_instruction = cur_block.instructions.at(k);

				new_instruction.off_source_loc_desc = original_instruction.off_source_loc_desc;
				new_instruction.set_opcode(original_instruction.opcode);
				new_instruction.set_output(original_instruction.output);
				if (!new_instruction.reserve_operands(output_allocator, original_instruction.num_operands))
					return OutOfMemoryError::alloc();
				memcpy(new_instruction.operands, original_instruction.operands, sizeof(Value) * new_instruction.num_operands);

				switch (new_instruction.opcode) {
					case Opcode::JMP: {
						const uint32_t dest = new_instruction.operands[0].get_u32();
						new_instruction.operands[0] = Value(ValueType::Label, basic_block_map.at(dest));
						break;
					}
					case Opcode::BR: {
						new_instruction.operands[1] = Value(ValueType::Label, basic_block_map.at(new_instruction.operands[1].get_u32()));
						new_instruction.operands[2] = Value(ValueType::Label, basic_block_map.at(new_instruction.operands[2].get_u32()));
						break;
					}
					default:
						break;
				}
			}

			control_flow_graph_out.basic_blocks.at(i) = std::move(cur_block);
		}
	}

	return {};
}
