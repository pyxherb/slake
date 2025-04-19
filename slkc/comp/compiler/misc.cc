#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::reindexFnParams(
	CompileContext* compileContext,
	peff::SharedPtr<FnNode> fn) {
	for (size_t i = 0; i < fn->params.size(); ++i) {
		peff::SharedPtr<VarNode> &curParam = fn->params.at(i);
		if (fn->paramIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::ParamAlreadyDefined)));
		}

		if (!fn->paramIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	fn->isParamsIndexed = true;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::indexFnParams(
	CompileContext *compileContext,
	peff::SharedPtr<FnNode> fn) {
	if (fn->isParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexFnParams(compileContext, fn));
	fn->isParamsIndexed = true;

	return {};
}
