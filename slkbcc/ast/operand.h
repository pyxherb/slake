#ifndef _SLKBCC_OPERAND_H_
#define _SLKBCC_OPERAND_H_

#include "ref.h"
#include <slake/reg.h>

namespace slake {
	namespace bcc {
		enum class OperandType : uint8_t {
			NONE,
			I8,
			I16,
			I32,
			I64,
			U8,
			U16,
			U32,
			U64,
			F32,
			F64,
			STRING,
			BOOL,
			ARRAY,
			MAP,
			REF,
			LABEL,
			TYPENAME,
			LVAR,
			REG,
			ARG
		};

		class Operand : public ILocated {
		private:
			location _loc;

		public:
			bool dereferenced = false;

			inline Operand(location loc) : _loc(loc) {}
			virtual ~Operand() = default;
			virtual OperandType getOperandType() const = 0;

			virtual location getLocation() const override { return _loc; }
		};

		class NullOperand : public Operand {
		public:
			inline NullOperand(location loc) : Operand(loc) {}
			virtual ~NullOperand() = default;
			virtual inline OperandType getOperandType() const override {
				return OperandType::NONE;
			}
		};

		template <typename T, OperandType OT>
		class LiteralOperand : public Operand {
		public:
			T data;

			inline LiteralOperand(location loc) : Operand(loc) {}
			inline LiteralOperand(location loc, T data) : Operand(loc), data(data) {}
			virtual ~LiteralOperand() = default;
			virtual inline OperandType getOperandType() const override {
				return OT;
			}
		};

		using I8Operand = LiteralOperand<int8_t, OperandType::I8>;
		using I16Operand = LiteralOperand<int16_t, OperandType::I16>;
		using I32Operand = LiteralOperand<int32_t, OperandType::I32>;
		using I64Operand = LiteralOperand<int64_t, OperandType::I64>;
		using U8Operand = LiteralOperand<uint8_t, OperandType::U8>;
		using U16Operand = LiteralOperand<uint16_t, OperandType::U16>;
		using U32Operand = LiteralOperand<uint32_t, OperandType::U32>;
		using U64Operand = LiteralOperand<uint64_t, OperandType::U64>;
		using F32Operand = LiteralOperand<float, OperandType::F32>;
		using F64Operand = LiteralOperand<double, OperandType::F64>;
		using StringOperand = LiteralOperand<string, OperandType::STRING>;
		using BoolOperand = LiteralOperand<bool, OperandType::BOOL>;
		using ArrayOperand = LiteralOperand<deque<shared_ptr<Operand>>, OperandType::ARRAY>;
		using MapOperand = LiteralOperand<deque<pair<shared_ptr<Operand>, shared_ptr<Operand>>>, OperandType::MAP>;
		using RefOperand = LiteralOperand<shared_ptr<Ref>, OperandType::REF>;
		using LabelOperand = LiteralOperand<string, OperandType::LABEL>;
		using TypeNameOperand = LiteralOperand<shared_ptr<TypeName>, OperandType::TYPENAME>;
		using LocalVarOperand = LiteralOperand<uint32_t, OperandType::LVAR>;
		using RegOperand = LiteralOperand<std::string, OperandType::REG>;
		using ArgOperand = LiteralOperand<uint32_t, OperandType::ARG>;
	}
}

#endif
