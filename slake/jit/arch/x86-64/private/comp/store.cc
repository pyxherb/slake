#include "store.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <size_t size>
InternalExceptionPointer compile_reg_to_field_var_store_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins,
	FieldRecord &field_record,
	char *raw_data_ptr,
	uint32_t reg_off) noexcept {
	InternalExceptionPointer exception;
	VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(reg_off);

	if (vreg_state.save_offset != INT32_MIN) {
		RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
		int32_t tmp_gp_reg_off = INT32_MIN;
		size_t tmp_gp_reg_size;

		if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
		}

		if constexpr (size == 1) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg8_ins(
															tmp_gp_reg_id,
															MemoryLocation{
																REG_RBP, vreg_state.save_offset,
																REG_MAX, 0 })));
		} else if constexpr (size == 2) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg16_ins(
															tmp_gp_reg_id,
															MemoryLocation{
																REG_RBP, vreg_state.save_offset,
																REG_MAX, 0 })));
		} else if constexpr (size == 4) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg32_ins(
															tmp_gp_reg_id,
															MemoryLocation{
																REG_RBP, vreg_state.save_offset,
																REG_MAX, 0 })));
		} else if constexpr (size == 8) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg64_ins(
															tmp_gp_reg_id,
															MemoryLocation{
																REG_RBP, vreg_state.save_offset,
																REG_MAX, 0 })));
		} else {
			static_assert(size != size, "Invalid operand size");
		}
		if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
			if constexpr (size == 1) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																tmp_gp_reg_id)));
			} else if constexpr (size == 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																tmp_gp_reg_id)));
			} else if constexpr (size == 4) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																tmp_gp_reg_id)));
			} else if constexpr (size == 8) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																tmp_gp_reg_id)));
			} else {
				static_assert(size != size, "Invalid operand size");
			}
		} else {
			RegisterId base_reg_id = compile_context.alloc_gp_reg();
			int32_t base_reg_off = INT32_MIN;
			size_t base_reg_size;

			if (compile_context.is_reg_in_use(base_reg_id)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
			}

			uint64_t imm0 = (uint64_t)raw_data_ptr;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
															base_reg_id,
															(uint8_t *)&imm0)));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
															MemoryLocation{
																base_reg_id, 0,
																REG_MAX, 0 },
															tmp_gp_reg_id)));

			if (base_reg_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(base_reg_id, base_reg_off, base_reg_size));
			}
		}

		if (tmp_gp_reg_off != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
		}
	} else {
		if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
			if constexpr (size == 1) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																vreg_state.phy_reg)));
			} else if constexpr (size == 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																vreg_state.phy_reg)));
			} else if constexpr (size == 4) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																vreg_state.phy_reg)));
			} else if constexpr (size == 8) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																	REG_MAX, 0 },
																vreg_state.phy_reg)));
			}
		} else {
			RegisterId base_reg_id = compile_context.alloc_gp_reg();

			if (base_reg_id == vreg_state.phy_reg) {
				int32_t vreg_reg_off = INT32_MIN;
				size_t vreg_reg_size;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(vreg_state.phy_reg, vreg_reg_off, vreg_reg_size));

				uint64_t imm0 = (uint64_t)raw_data_ptr;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																base_reg_id,
																(uint8_t *)&imm0)));

				RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
				int32_t tmp_gp_reg_off = INT32_MIN;
				size_t tmp_gp_reg_size;

				if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
				}

				if constexpr (size == 1) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg8_ins(
																	tmp_gp_reg_id,
																	MemoryLocation{
																		REG_RBP, vreg_state.save_offset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	tmp_gp_reg_id)));
				} else if constexpr (size == 2) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg16_ins(
																	tmp_gp_reg_id,
																	MemoryLocation{
																		REG_RBP, vreg_state.save_offset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	tmp_gp_reg_id)));
				} else if constexpr (size == 4) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg32_ins(
																	tmp_gp_reg_id,
																	MemoryLocation{
																		REG_RBP, vreg_state.save_offset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	tmp_gp_reg_id)));
				} else if constexpr (size == 8) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg64_ins(
																	tmp_gp_reg_id,
																	MemoryLocation{
																		REG_RBP, vreg_state.save_offset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	tmp_gp_reg_id)));
				}

				if (tmp_gp_reg_off != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
				}

			} else {
				int32_t base_reg_off = INT32_MIN;
				size_t base_reg_size;

				if (compile_context.is_reg_in_use(base_reg_id)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
				}

				uint64_t imm0 = (uint64_t)raw_data_ptr;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																base_reg_id,
																(uint8_t *)&imm0)));

				if constexpr (size == 1) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	vreg_state.phy_reg)));
				} else if constexpr (size == 2) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	vreg_state.phy_reg)));
				} else if constexpr (size == 4) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	vreg_state.phy_reg)));
				} else if constexpr (size == 8) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																	MemoryLocation{
																		base_reg_id, 0,
																		REG_MAX, 0 },
																	vreg_state.phy_reg)));
				}

				if (base_reg_off != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(base_reg_id, base_reg_off, base_reg_size));
				}
			}

			compile_context.unalloc_reg(base_reg_id);
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compile_store_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins) noexcept {
	InternalExceptionPointer exception;
	opti::RegAnalyzedInfo &reg_analyzed_info = analyzed_info.analyzed_reg_info.at(cur_ins.operands[0].get_reg_index());
	Value rhs = cur_ins.operands[1];

	switch (reg_analyzed_info.storage_type) {
		case opti::RegStorageType::None:
			std::terminate();
		case opti::RegStorageType::FieldVar: {
			const Reference &entity_ref = reg_analyzed_info.expected_value.get_reference();
			FieldRecord &field_record = entity_ref.as_static_field.module_object->field_records.at(entity_ref.as_static_field.index);
			char *raw_data_ptr = entity_ref.as_static_field.module_object->local_field_storage.data() + field_record.offset;

			switch (field_record.type.type_id) {
				case TypeId::I8:
				case TypeId::I16:
				case TypeId::I32:
				case TypeId::I64:
				case TypeId::U8:
				case TypeId::U16:
				case TypeId::U32:
				case TypeId::U64:
				case TypeId::F32:
				case TypeId::F64:
				case TypeId::Bool: {
					if (rhs.value_type == ValueType::RegIndex) {
						switch (field_record.type.type_id) {
							case TypeId::I8:
							case TypeId::U8:
							case TypeId::Bool: {
								compile_reg_to_field_var_store_instruction<1>(compile_context, analyzed_info, off_ins, cur_ins, field_record, raw_data_ptr, rhs.get_reg_index());
								break;
							}
							case TypeId::I16:
							case TypeId::U16: {
								compile_reg_to_field_var_store_instruction<2>(compile_context, analyzed_info, off_ins, cur_ins, field_record, raw_data_ptr, rhs.get_reg_index());
								break;
							}
							case TypeId::I32:
							case TypeId::U32:
							case TypeId::F32: {
								compile_reg_to_field_var_store_instruction<4>(compile_context, analyzed_info, off_ins, cur_ins, field_record, raw_data_ptr, rhs.get_reg_index());
								break;
							}
							case TypeId::I64:
							case TypeId::U64:
							case TypeId::F64: {
								compile_reg_to_field_var_store_instruction<8>(compile_context, analyzed_info, off_ins, cur_ins, field_record, raw_data_ptr, rhs.get_reg_index());
								break;
							}
							default:
								std::terminate();
						}
					} else {
						switch (field_record.type.type_id) {
							case TypeId::I8: {
								int8_t imm0 = rhs.get_i8();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm8_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::I16: {
								int16_t imm0 = rhs.get_i16();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm16_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::I32: {
								int32_t imm0 = rhs.get_i32();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm32_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::I64: {
								int64_t imm0 = rhs.get_i64();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm64_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::U8: {
								uint8_t imm0 = rhs.get_u8();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm8_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::U16: {
								uint16_t imm0 = rhs.get_u16();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm16_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::U32: {
								uint16_t imm0 = rhs.get_u16();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm32_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::U64: {
								uint64_t imm0 = rhs.get_u64();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm64_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::Bool: {
								bool imm0 = rhs.get_bool();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm8_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::F32: {
								float imm0 = rhs.get_f32();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm32_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
							case TypeId::F64: {
								double imm0 = rhs.get_f64();
								if (((uintptr_t)raw_data_ptr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																					emit_mov_imm64_to_mem_ins(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)raw_data_ptr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId base_reg_id = compile_context.alloc_gp_reg();

									int32_t base_reg_off = INT32_MIN;
									size_t base_reg_size;

									uint64_t base_reg_imm0 = (uint64_t)raw_data_ptr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(
																					base_reg_id,
																					(uint8_t *)&base_reg_imm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_mem_ins(
																					MemoryLocation{
																						base_reg_id, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compile_context.is_reg_in_use(base_reg_id)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(base_reg_id, base_reg_off, base_reg_size));
									}

									compile_context.unalloc_reg(base_reg_id);
								}
								break;
							}
						}
					}
				}
			}
			break;
		}
		case opti::RegStorageType::LocalVar: {
			/*LocalVarState &local_var_state = compile_context.local_var_states.at(reg_analyzed_info.expected_value.get_reference().as_local_var.local_var_index);

			switch (local_var_state.type.type_id) {
			case TypeId::Value: {
				if (rhs.value_type == ValueType::RegIndex) {
					switch (local_var_state.type.get_value_type_ex_data()) {
					case TypeId::I8:
					case TypeId::U8:
					case TypeId::Bool: {
						uint32_t reg_off = rhs.get_reg_index();
						VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(reg_off);

						if (vreg_state.save_offset != INT32_MIN) {
							RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
							int32_t tmp_gp_reg_off = INT32_MIN;
							size_t tmp_gp_reg_size;

							if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg8_ins(
																			tmp_gp_reg_id,
																			MemoryLocation{
																				REG_RBP, vreg_state.save_offset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			tmp_gp_reg_id)));

							if (tmp_gp_reg_off != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			vreg_state.phy_reg)));
						}
						break;
					}
					case TypeId::I16:
					case TypeId::U16: {
						uint32_t reg_off = rhs.get_reg_index();
						VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(reg_off);

						if (vreg_state.save_offset != INT32_MIN) {
							RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
							int32_t tmp_gp_reg_off = INT32_MIN;
							size_t tmp_gp_reg_size;

							if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg16_ins(
																			tmp_gp_reg_id,
																			MemoryLocation{
																				REG_RBP, vreg_state.save_offset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			tmp_gp_reg_id)));

							if (tmp_gp_reg_off != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			vreg_state.phy_reg)));
						}
						break;
					}
					case TypeId::I32:
					case TypeId::U32:
					case TypeId::F32: {
						uint32_t reg_off = rhs.get_reg_index();
						VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(reg_off);

						if (vreg_state.save_offset != INT32_MIN) {
							RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
							int32_t tmp_gp_reg_off = INT32_MIN;
							size_t tmp_gp_reg_size;

							if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg32_ins(
																			tmp_gp_reg_id,
																			MemoryLocation{
																				REG_RBP, vreg_state.save_offset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			tmp_gp_reg_id)));

							if (tmp_gp_reg_off != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			vreg_state.phy_reg)));
						}
						break;
					}
					case TypeId::I64:
					case TypeId::U64:
					case TypeId::F64: {
						uint32_t reg_off = rhs.get_reg_index();
						VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(reg_off);

						if (vreg_state.save_offset != INT32_MIN) {
							RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
							int32_t tmp_gp_reg_off = INT32_MIN;
							size_t tmp_gp_reg_size;

							if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg64_ins(
																			tmp_gp_reg_id,
																			MemoryLocation{
																				REG_RBP, vreg_state.save_offset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			tmp_gp_reg_id)));

							if (tmp_gp_reg_off != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			vreg_state.phy_reg)));
						}
						break;
					}
					}
				} else {
					switch (local_var_state.type.get_value_type_ex_data()) {
					case TypeId::I8: {
						int8_t imm0 = rhs.get_i8();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm8_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::I16: {
						int16_t imm0 = rhs.get_i16();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm16_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::I32: {
						int32_t imm0 = rhs.get_i32();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm32_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::I64: {
						int64_t imm0 = rhs.get_i64();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm64_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U8: {
						uint8_t imm0 = rhs.get_u8();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm8_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U16: {
						uint16_t imm0 = rhs.get_u16();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm16_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U32: {
						uint16_t imm0 = rhs.get_u16();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm16_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U64: {
						uint64_t imm0 = rhs.get_u64();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm64_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::Bool: {
						bool imm0 = rhs.get_bool();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm8_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::F32: {
						float imm0 = rhs.get_f32();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm32_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::F64: {
						double imm0 = rhs.get_f64();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_imm64_to_mem_ins(
																			MemoryLocation{
																				REG_RBP, local_var_state.stack_off,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					}
				}
				break;
			}
			case TypeId::String:
			case TypeId::Instance:
			case TypeId::Array:
			case TypeId::FnDelegate: {
				if (rhs.value_type == ValueType::RegIndex) {
					uint32_t reg_off = rhs.get_reg_index();
					VirtualRegState &vreg_state = compile_context.virtual_reg_states.at(reg_off);

					if (vreg_state.save_offset != INT32_MIN) {
						RegisterId tmp_gp_reg_id = compile_context.alloc_gp_reg();
						int32_t tmp_gp_reg_off = INT32_MIN;
						size_t tmp_gp_reg_size;

						if (compile_context.is_reg_in_use(tmp_gp_reg_id)) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
						}

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_mem_to_reg64_ins(
																		tmp_gp_reg_id,
																		MemoryLocation{
																			REG_RBP, vreg_state.save_offset,
																			REG_MAX, 0 })));
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																		MemoryLocation{
																			REG_RBP, local_var_state.stack_off,
																			REG_MAX, 0 },
																		tmp_gp_reg_id)));

						if (tmp_gp_reg_off != INT32_MIN) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_gp_reg_id, tmp_gp_reg_off, tmp_gp_reg_size));
						}
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_mem_ins(
																		MemoryLocation{
																			REG_RBP, local_var_state.stack_off,
																			REG_MAX, 0 },
																		vreg_state.phy_reg)));
					}
				} else {
					Object *imm0 = rhs.get_reference().as_object;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																	emit_mov_imm64_to_mem_ins(
																		MemoryLocation{
																			REG_RBP, local_var_state.stack_off,
																			REG_MAX, 0 },
																		(uint8_t *)&imm0)));
				}
				break;
			}
			}
			break;*/
		}
		case opti::RegStorageType::ArgRef:
			// TODO: Implement it.
			break;
	}

	return {};
}
