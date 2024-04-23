#include "../compiler.h"

using namespace slake::slkc;

uint32_t Compiler::allocLocalVar(string name, shared_ptr<TypeNameNode> type) {
	if (curMajorContext.curMinorContext.localVars.size() > UINT32_MAX)
		throw FatalCompilationError(
			Message(
				type->getLocation(),
				MessageType::Error,
				"Number limit of local variables exceeded"));

	uint32_t index = (uint32_t)curMajorContext.curMinorContext.localVars.size();

	curMajorContext.curMinorContext.localVars[name] = make_shared<LocalVarNode>(index, type);
	curFn->insertIns(Opcode::LVAR, type);

	return index;
}

uint32_t Compiler::allocReg(uint32_t nRegs) {
	auto idxReg = curMajorContext.curRegCount;

	curFn->insertIns(Opcode::REG, make_shared<U32LiteralExprNode>(Location(), nRegs));

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
