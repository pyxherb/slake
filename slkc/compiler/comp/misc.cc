#include "../compiler.h"

#undef max

using namespace slake::slkc;

uint32_t CompileContext::allocLocalVar(std::string name, std::shared_ptr<TypeNameNode> type) {
	if (curTopLevelContext.curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	if (curTopLevelContext.curMajorContext.curMinorContext.localVars.size() > UINT32_MAX)
		throw FatalCompilationError(
			Message(
				compiler->tokenRangeToSourceLocation(type->tokenRange),
				MessageType::Error,
				"Exceeded maximum number of local variables"));

	uint32_t refRegId = allocReg();

	curTopLevelContext.curMajorContext.curMinorContext.localVars[name] = std::make_shared<LocalVarNode>(name, refRegId, type);
	_insertIns(Opcode::LVAR, std::make_shared<RegRefNode>(refRegId), { type });

	return refRegId;
}

uint32_t CompileContext::allocReg() {
	if (curTopLevelContext.curMajorContext.curMinorContext.dryRun)
		return UINT32_MAX;

	auto idxReg = curTopLevelContext.curRegCount++;

	curFn->maxRegCount = std::max(curFn->maxRegCount, idxReg + 1);

	return idxReg;
}

void CompileContext::pushTopLevelContext() {
	savedTopLevelContexts.push_back(curTopLevelContext);
}

void CompileContext::popTopLevelContext() {
	curTopLevelContext = savedTopLevelContexts.back();
	savedTopLevelContexts.pop_back();
}

void CompileContext::pushMajorContext() {
	curTopLevelContext.savedMajorContexts.push_back(curTopLevelContext.curMajorContext);
}

void CompileContext::popMajorContext() {
	curTopLevelContext.curMajorContext = curTopLevelContext.savedMajorContexts.back();
	curTopLevelContext.savedMajorContexts.pop_back();
}

void CompileContext::pushMinorContext() {
	curTopLevelContext.curMajorContext.pushMinorContext();
}

void CompileContext::popMinorContext() {
	curTopLevelContext.curMajorContext.popMinorContext();
}
