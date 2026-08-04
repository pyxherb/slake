#ifndef _SLAKE_UTIL_BC_BUILDER_H_
#define _SLAKE_UTIL_BC_BUILDER_H_

#include <slake/obj/fn.h>

namespace slake {
	class BCBuilder final {
	private:
		Runtime *_rt;

	public:
		using BCLabelId = uint32_t;

		HostObjectRef<RegularFnOverloadingObject> fn_object;
		peff::Map<BCLabelId, uint32_t> label_to_offset_map;
		BCLabelId cur_label_id = 0;
		peff::Map<std::pair<uint32_t, bool>, BCLabelId> operand_to_label_replacement_map;

		SLAKE_API BCBuilder(Runtime *rt, peff::Alloc *resource_allocator);
		SLAKE_API ~BCBuilder();

		SLAKE_FORCEINLINE void set_fn(RegularFnOverloadingObject *fn_object) noexcept {
			commit();
			this->fn_object = fn_object;
		}

		SLAKE_FORCEINLINE RegularFnOverloadingObject *get_fn() const noexcept {
			return fn_object.get();
		}

		struct RegLocation {
			InsRegType reg_type = InsRegType::Any;
			RegIndex index = INVALID_REG;
		};

		enum class OperandClass : uint8_t {
			Immediate = 0,
			Label
		};

		struct Operand {
			union {
				uint64_t as_imm;
				BCLabelId as_label;
			};
			OperandClass operand_class;

			SLAKE_FORCEINLINE static Operand new_imm(uint64_t imm) noexcept {
				Operand op;
				op.as_imm = imm;
				op.operand_class = OperandClass::Immediate;
				return op;
			}

			SLAKE_FORCEINLINE static Operand new_label(BCLabelId label_id) noexcept {
				Operand op;
				op.as_label = label_id;
				op.operand_class = OperandClass::Label;
				return op;
			}
		};

		SLAKE_FORCEINLINE peff::Option<BCLabelId> alloc_label() {
			if(!label_to_offset_map.insert(+cur_label_id, UINT32_MAX))
				return peff::NULLOPT;
			return cur_label_id++;
		}

		SLAKE_FORCEINLINE void set_label_off(BCLabelId label_id, uint32_t off) {
			label_to_offset_map.at(label_id) = off;
		}

		SLAKE_FORCEINLINE bool emit_ins(
			Opcode opcode,
			uint8_t flags,
			RegLocation reg_out,
			RegLocation reg1,
			RegLocation reg2,
			const Operand &operand0,
			const Operand &operand1) noexcept {
			Instruction ins = {};

			ins.opcode = opcode;
			ins.flags = flags;
			ins.reg_out_type = static_cast<uint8_t>(reg_out.reg_type);
			ins.reg_out = reg_out.index;
			ins.reg0_type = static_cast<uint8_t>(reg1.reg_type);
			ins.reg0 = reg1.index;
			ins.reg1_type = static_cast<uint8_t>(reg2.reg_type);
			ins.reg1 = reg2.index;

			switch (operand0.operand_class) {
				case OperandClass::Immediate:
					ins.operands[0] = operand0.as_imm;
					break;
				case OperandClass::Label: {
					ins.operands[0] = UINT64_MAX;
					if (!operand_to_label_replacement_map.insert({fn_object->instructions.size(), false}, +operand0.as_label))
						return false;
					break;
				}
				default:
					break;
			}

			switch (operand1.operand_class) {
				case OperandClass::Immediate:
					ins.operands[1] = operand1.as_imm;
					break;
				case OperandClass::Label: {
					ins.operands[1] = UINT64_MAX;
					if (!operand_to_label_replacement_map.insert({fn_object->instructions.size(), true}, +operand1.as_label))
						return false;
					break;
				}
				default:
					break;
			}

			if (!fn_object->instructions.push_back(std::move(ins)))
				return false;

			return true;
		}

		SLAKE_FORCEINLINE void commit() noexcept {
			for(auto [k, v] : operand_to_label_replacement_map) {
				assert(v < sizeof(Instruction::operands) / sizeof(Instruction::operands[0]));
				fn_object->instructions.at(k.first).operands[k.second ? 1 : 0] = label_to_offset_map.at(v);
			}
			operand_to_label_replacement_map.clear();
		}
	};
}

#endif
