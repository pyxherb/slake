#include "../compiler.h"

using namespace slake::slkc;

uint32_t CompileContext::allocLocalVar(std::string name, std::shared_ptr<TypeNameNode> type) {
	if (curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	if (curMajorContext.curMinorContext.localVars.size() > UINT32_MAX)
		throw FatalCompilationError(
			Message(
				compiler->tokenRangeToSourceLocation(type->tokenRange),
				MessageType::Error,
				"Exceeded maximum number of local variables"));

	uint32_t index = (uint32_t)curMajorContext.curMinorContext.localVars.size();

	curMajorContext.curMinorContext.localVars[name] = std::make_shared<LocalVarNode>(name, index, type);
	_insertIns(Opcode::LVAR, {}, { type });

	return index;
}

uint32_t CompileContext::allocReg() {
	if (curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	auto idxReg = curMajorContext.curRegCount;

	_insertIns(Opcode::REG, {}, { std::make_shared<U32LiteralExprNode>(curMajorContext.curRegCount++) });

	return idxReg;
}

void CompileContext::pushMajorContext() {
	savedMajorContexts.push_back(curMajorContext);
}

void CompileContext::popMajorContext() {
	curMajorContext = savedMajorContexts.back();
	savedMajorContexts.pop_back();
}

void CompileContext::pushMinorContext() {
	curMajorContext.pushMinorContext();
}

void CompileContext::popMinorContext() {
	curMajorContext.popMinorContext();
}
