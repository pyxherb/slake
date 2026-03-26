#include "mov.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

InternalExceptionPointer slake::jit::x86_64::compile_mov_instruction(
	JITCompileContext &compile_context,
	opti::ProgramAnalyzedInfo &analyzed_info,
	size_t off_ins,
	const Instruction &cur_ins) noexcept {
	InternalExceptionPointer exception;
	uint32_t output_reg_index = cur_ins.output;

	Value src = cur_ins.operands[1];

	switch (src.value_type) {
		case ValueType::I8: {
			int8_t imm0 = src.get_i8();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(int8_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::I16: {
			int16_t imm0 = src.get_i16();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_reg16_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(int16_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::I32: {
			int32_t imm0 = src.get_i32();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(int32_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::I64: {
			int64_t imm0 = src.get_i64();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(int64_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::U8: {
			uint8_t imm0 = src.get_u8();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint8_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::U16: {
			uint16_t imm0 = src.get_u16();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm16_to_reg16_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint16_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::U32: {
			uint32_t imm0 = src.get_u32();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint32_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::U64: {
			uint64_t imm0 = src.get_u64();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint64_t));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::Bool: {
			bool imm0 = src.get_bool();

			RegisterId reg_id = compile_context.alloc_gp_reg();
			if (compile_context.is_reg_in_use(reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm8_to_reg8_ins(reg_id, (uint8_t *)&imm0)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(bool));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			break;
		}
		case ValueType::F32: {
			float imm0 = src.get_f32();

			RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
			int32_t tmp_reg_off = INT32_MIN;
			size_t tmp_reg_size;
			if (compile_context.is_reg_in_use(tmp_reg_id)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(tmp_reg_id, (uint8_t *)&imm0)));

			RegisterId xmm_reg_id = compile_context.alloc_xmm_reg();
			if (compile_context.is_reg_in_use(xmm_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg_xmm(xmm_reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movd_reg32_to_reg_xmm_ins(xmm_reg_id, tmp_reg_id)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, xmm_reg_id, sizeof(float));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			if (tmp_reg_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
			}
			break;
		}
		case ValueType::F64: {
			double imm0 = src.get_f64();

			RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
			int32_t tmp_reg_off = INT32_MIN;
			size_t tmp_reg_size;
			if (compile_context.is_reg_in_use(tmp_reg_id)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm32_to_reg32_ins(tmp_reg_id, (uint8_t *)&imm0)));

			RegisterId xmm_reg_id = compile_context.alloc_xmm_reg();
			if (compile_context.is_reg_in_use(xmm_reg_id)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg_xmm(xmm_reg_id, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movd_reg32_to_reg_xmm_ins(xmm_reg_id, tmp_reg_id)));
			VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, xmm_reg_id, sizeof(double));
			if (!output_vreg_state)
				return OutOfMemoryError::alloc();
			if (tmp_reg_off != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
			}
			break;
		}
		case ValueType::Reference: {
			Reference entity_ref = src.get_reference();

			switch (entity_ref.kind) {
				case ReferenceKind::ObjectRef: {
					Object *imm0 = entity_ref.as_object;

					RegisterId reg_id = compile_context.alloc_gp_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
					}
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(reg_id, (uint8_t *)&imm0)));
					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(void *));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case ValueType::RegIndex: {
			uint32_t src_reg_index = src.get_reg_index();
			auto &src_reg_info = analyzed_info.analyzed_reg_info.at(src_reg_index);
			auto &src_vreg_info = compile_context.virtual_reg_states.at(src_reg_index);
			TypeRef &src_reg_type = src_reg_info.type;
			switch (src_reg_type.type_id) {
				case TypeId::I8:
				case TypeId::U8:
				case TypeId::Bool: {
					RegisterId reg_id = compile_context.alloc_gp_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_mem_to_reg8_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg8_to_reg8_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint8_t));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::I16:
				case TypeId::U16: {
					RegisterId reg_id = compile_context.alloc_gp_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_mem_to_reg16_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg16_to_reg16_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint16_t));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::I32:
				case TypeId::U32: {
					RegisterId reg_id = compile_context.alloc_gp_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_mem_to_reg32_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg32_to_reg32_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint32_t));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::I64:
				case TypeId::U64: {
					RegisterId reg_id = compile_context.alloc_gp_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_mem_to_reg64_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint64_t));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::F32: {
					RegisterId reg_id = compile_context.alloc_xmm_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg_xmm(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_movd_mem_to_reg_xmm_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_reg_xmm_to_reg_xmm_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(float));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::F64: {
					RegisterId reg_id = compile_context.alloc_xmm_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg_xmm(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_movq_mem_to_reg_xmm_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_movq_reg_xmm_to_reg_xmm_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(double));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				} /*
				 case TypeId::Reference: {
					 int32_t off;
					 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.stack_alloc_aligned(sizeof(Reference), sizeof(Reference), off));

					 {
						 RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
						 int32_t tmp_reg_off = INT32_MIN;
						 size_t tmp_reg_size;
						 if (compile_context.is_reg_in_use(tmp_reg_id)) {
							 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						 }

						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(tmp_reg_id, REG_RBP)));
						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_add_imm32_to_reg64_ins(tmp_reg_id, (uint8_t *)&src_vreg_info.save_offset)));

						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(REG_RCX, tmp_reg_id)));

						 if (tmp_reg_off != INT32_MIN) {
							 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						 }
					 }
					 {
						 RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
						 int32_t tmp_reg_off = INT32_MIN;
						 size_t tmp_reg_size;
						 if (compile_context.is_reg_in_use(tmp_reg_id)) {
							 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						 }

						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(tmp_reg_id, REG_RBP)));
						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_add_imm32_to_reg64_ins(tmp_reg_id, (uint8_t *)&off)));

						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(REG_RDX, tmp_reg_id)));

						 if (tmp_reg_off != INT32_MIN) {
							 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						 }
					 }
					 {
						 uint64_t size = sizeof(Reference);
						 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(REG_R8, (uint8_t *)&size)));
					 }

					 CallingRegSavingInfo calling_reg_saving_info;

					 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.save_calling_regs(calling_reg_saving_info));

					 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_call_ins((void *)memcpy_wrapper)));

					 SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.restore_calling_regs(calling_reg_saving_info));

					 VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, off, sizeof(double));
					 if (!output_vreg_state)
						 return OutOfMemoryError::alloc();
					 break;
				 }*/
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn: {
					RegisterId reg_id = compile_context.alloc_gp_reg();
					if (compile_context.is_reg_in_use(reg_id)) {
						int32_t off;
						size_t size;
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(reg_id, off, size));
					}
					if (src_vreg_info.save_offset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(
																		emit_mov_mem_to_reg64_ins(
																			reg_id,
																			MemoryLocation{ REG_RBP, src_vreg_info.save_offset, REG_MAX, 0 })));
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(reg_id, src_vreg_info.phy_reg)));
					}

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, reg_id, sizeof(uint64_t));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::Ref: {
					switch (src_reg_info.storage_type) {
						case opti::RegStorageType::None:
							break;
						case opti::RegStorageType::FieldVar:
							if (src_reg_info.storage_info.as_field_var.is_used_for_output) {
							} else {
								VirtualRegState *output_vreg_state = compile_context.def_dummy_virtual_reg(output_reg_index);
								if (!output_vreg_state)
									return OutOfMemoryError::alloc();
							}
							break;
						case opti::RegStorageType::LocalVar:
							if (src_reg_info.storage_info.as_local_var.is_used_for_output) {
							} else {
								VirtualRegState *output_vreg_state = compile_context.def_dummy_virtual_reg(output_reg_index);
								if (!output_vreg_state)
									return OutOfMemoryError::alloc();
							}
							break;
						case opti::RegStorageType::ArgRef:
							if (src_reg_info.storage_info.as_arg_ref.is_used_for_output) {
							} else {
								VirtualRegState *output_vreg_state = compile_context.def_dummy_virtual_reg(output_reg_index);
								if (!output_vreg_state)
									return OutOfMemoryError::alloc();
							}
							break;
					}
					break;
				}
				case TypeId::Any: {
					int32_t off;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.stack_alloc_aligned(sizeof(Value), sizeof(Value), off));

					{
						RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
						int32_t tmp_reg_off = INT32_MIN;
						size_t tmp_reg_size;
						if (compile_context.is_reg_in_use(tmp_reg_id)) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						}

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(tmp_reg_id, REG_RBP)));
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_add_imm32_to_reg64_ins(tmp_reg_id, (uint8_t *)&src_vreg_info.save_offset)));

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(REG_RCX, tmp_reg_id)));

						if (tmp_reg_off != INT32_MIN) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						}
					}
					{
						RegisterId tmp_reg_id = compile_context.alloc_gp_reg();
						int32_t tmp_reg_off = INT32_MIN;
						size_t tmp_reg_size;
						if (compile_context.is_reg_in_use(tmp_reg_id)) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						}

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(tmp_reg_id, REG_RBP)));
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_add_imm32_to_reg64_ins(tmp_reg_id, (uint8_t *)&off)));

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_reg64_to_reg64_ins(REG_RDX, tmp_reg_id)));

						if (tmp_reg_off != INT32_MIN) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.pop_reg(tmp_reg_id, tmp_reg_off, tmp_reg_size));
						}
					}
					{
						uint64_t size = sizeof(Value);
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_mov_imm64_to_reg64_ins(REG_R8, (uint8_t *)&size)));
					}

					CallingRegSavingInfo calling_reg_saving_info;

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.save_calling_regs(calling_reg_saving_info));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.push_ins(emit_call_ins((void *)memcpy_wrapper)));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compile_context.restore_calling_regs(calling_reg_saving_info));

					VirtualRegState *output_vreg_state = compile_context.def_virtual_reg(output_reg_index, off, sizeof(double));
					if (!output_vreg_state)
						return OutOfMemoryError::alloc();

					break;
				} break;
			}
			break;
		}
		default:
			// Unhandled value type
			std::terminate();
	}

	return {};
}
