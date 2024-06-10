#include "../compiler.h"

using namespace slake::slkc;

uint32_t Compiler::allocLocalVar(std::string name, std::shared_ptr<TypeNameNode> type) {
	if (curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	if (curMajorContext.curMinorContext.localVars.size() > UINT32_MAX)
		throw FatalCompilationError(
			Message(
				type->sourceLocation,
				MessageType::Error,
				"Exceeded maximum number of local variables"));

	uint32_t index = (uint32_t)curMajorContext.curMinorContext.localVars.size();

	curMajorContext.curMinorContext.localVars[name] = std::make_shared<LocalVarNode>(name, index, type);
	_insertIns(Opcode::LVAR, type);

	return index;
}

uint32_t Compiler::allocReg(uint32_t nRegs) {
	if (curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	auto idxReg = curMajorContext.curRegCount;

	_insertIns(Opcode::REG, std::make_shared<U32LiteralExprNode>(nRegs));

	curMajorContext.curRegCount += nRegs;

	return idxReg;
}

void Compiler::pushMajorContext() {
	_savedMajorContexts.push_back(curMajorContext);
}

void Compiler::popMajorContext() {
	curMajorContext = _savedMajorContexts.back();
	_savedMajorContexts.pop_back();
}

void Compiler::pushMinorContext() {
	curMajorContext.pushMinorContext();
}

void Compiler::popMinorContext() {
	curMajorContext.popMinorContext();
}
