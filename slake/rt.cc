#include "rt.h"

#include <sstream>

#include "base/byteord.hh"
#include "slxfmt.h"

using namespace Slake;

class SlakeFn : public Fn {
public:
	std::deque<Instruction> body;
};

class SlakeModule : public Module {
protected:
public:
	inline SlakeModule() {
	}
	virtual inline ~SlakeModule() {}

	virtual const std::shared_ptr<Fn> getFn(std::string name) override {
	}

	virtual void registerNativeVar(std::string name, const NativeVarRegistry& registry) override {
	}
	virtual void unregisterNativeVar(std::string name) override {
	}
};

class SlakeContext {
public:
	std::shared_ptr<SlakeModule> mod;
	std::shared_ptr<SlakeFn> fn;
	std::uint32_t currentIns = 0;
	std::deque<std::shared_ptr<Value>> stack;
	std::vector<std::uint32_t> stackFrameBases;

	inline SlakeContext(std::shared_ptr<SlakeModule> mod, std::shared_ptr<SlakeFn> fn) {
	}
	virtual inline ~SlakeContext() {}
};

class SlakeRuntime : public Runtime {
public:
	std::unordered_map<std::string, std::shared_ptr<Module>> modules;

	inline void checkOperandCount(std::shared_ptr<Instruction> ins, std::uint8_t max) {
		assert(max <= 3);
		if (ins->getOperandCount() > max)
			throw InvalidOperandsError("Invalid operand count");
	}

	inline void checkOperandType(
		std::shared_ptr<Instruction> ins,
		ValueType type0,
		ValueType type1 = ValueType::OBJECT,
		ValueType type2 = ValueType::OBJECT,
		ValueType type3 = ValueType::OBJECT) {
		if (ins->operands[0]->getType() != type0 ||
			(type1 != ValueType::OBJECT ? ins->operands[1]->getType() != type1 : true) ||
			(type2 != ValueType::OBJECT ? ins->operands[2]->getType() != type2 : true) ||
			(type3 != ValueType::OBJECT ? ins->operands[3]->getType() != type3 : true))
			throw InvalidOperandsError("Invalid operand combination");
	}

	void execIns(std::shared_ptr<SlakeContext> context, std::shared_ptr<Instruction> ins) {
		switch (ins->opcode) {
			case Opcode::NOP:
				checkOperandCount(ins, 0);
				break;
			case Opcode::PUSH:
				checkOperandCount(ins, 1);
				context->stack.push_back(ins->operands[0]);
				break;
			case Opcode::POP:
				checkOperandCount(ins, 0);
				context->stack.pop_back();
				break;
			case Opcode::LLOAD:
			case Opcode::LOAD:
				checkOperandCount(ins, 1);
				if (ins->opcode == Opcode::LLOAD) {
					checkOperandType(ins, ValueType::I32);
					context->stack.push_back(context->stack[*(context->stackFrameBases.rbegin())]);
				} else {
					checkOperandType(ins, ValueType::I32);
				}
				break;
			case Opcode::LSTORE:
			case Opcode::STORE:
				checkOperandCount(ins, 1);
				checkOperandType(ins, ValueType::I32);
				context->stack.pop_back();
				break;
			default:
				throw InvalidOpcodeError("Invalid opcode " + std::to_string((std::uint8_t)ins->opcode));
		}
	}
};

std::shared_ptr<Module> Slake::loadModule(const void* src, std::size_t size) {
	std::shared_ptr<SlakeModule> mod;
	std::stringbuf s;
	s.pubsetbuf((char*)src, size);

	{
		SlxFmt::ImgHeader ih;
		s.sgetn((char*)&ih, sizeof(ih));
		if ((ih.magic[0] != SlxFmt::IMH_MAGIC[0]) ||
			(ih.magic[1] != SlxFmt::IMH_MAGIC[1]) ||
			(ih.magic[2] != SlxFmt::IMH_MAGIC[2]) ||
			(ih.magic[3] != SlxFmt::IMH_MAGIC[3]))
			return std::shared_ptr<Module>();
		if (ih.fmtVer != 0)
			return std::shared_ptr<Module>();
		mod = std::make_shared<SlakeModule>();
	}
}
