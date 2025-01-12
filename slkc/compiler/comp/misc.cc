#include "../compiler.h"

#undef max

using namespace slake::slkc;

uint32_t CompileContext::allocLocalVar(std::string name, std::shared_ptr<TypeNameNode> type) {
	if (curCollectiveContext.curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	if (curCollectiveContext.curMajorContext.curMinorContext.localVars.size() > UINT32_MAX)
		throw FatalCompilationError(
			Message(
				compiler->tokenRangeToSourceLocation(type->tokenRange),
				MessageType::Error,
				"Exceeded maximum number of local variables"));

	uint32_t index = (uint32_t)curCollectiveContext.curMajorContext.curMinorContext.localVars.size();

	curCollectiveContext.curMajorContext.curMinorContext.localVars[name] = std::make_shared<LocalVarNode>(name, index, type);
	_insertIns(Opcode::LVAR, {}, { type });

	return index;
}

uint32_t CompileContext::allocReg() {
	if (curCollectiveContext.curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	auto idxReg = curCollectiveContext.curRegCount++;

	curFn->maxRegCount = std::max(curFn->maxRegCount, idxReg + 1);

	return idxReg;
}

void CompileContext::pushCollectiveContext() {
	savedCollectiveContexts.push_back(curCollectiveContext);
}

void CompileContext::popCollectiveContext() {
	curCollectiveContext = savedCollectiveContexts.back();
	savedCollectiveContexts.pop_back();
}

void CompileContext::pushMajorContext() {
	curCollectiveContext.savedMajorContexts.push_back(curCollectiveContext.curMajorContext);
}

void CompileContext::popMajorContext() {
	curCollectiveContext.curMajorContext = curCollectiveContext.savedMajorContexts.back();
	curCollectiveContext.savedMajorContexts.pop_back();
}

void CompileContext::pushMinorContext() {
	curCollectiveContext.curMajorContext.pushMinorContext();
}

void CompileContext::popMinorContext() {
	curCollectiveContext.curMajorContext.popMinorContext();
}
