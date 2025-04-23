#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::reindexFnParams(
	CompileContext *compileContext,
	peff::SharedPtr<FnOverloadingNode> fn) {
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
	peff::SharedPtr<FnOverloadingNode> fn) {
	if (fn->isParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexFnParams(compileContext, fn));
	fn->isParamsIndexed = true;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::renormalizeModuleVarDefStmts(
	CompileContext *compileContext,
	peff::SharedPtr<ModuleNode> mod) {
	for (auto &i : mod->varDefStmts) {
		for (auto &j : i->varDefEntries) {
			if (mod->memberIndices.contains(j->name)) {
				SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(/* TODO: Use variable definition entries' token range! */ i->tokenRange, CompilationErrorKind::MemberAlreadyDefined)));
			}
			peff::SharedPtr<VarNode> varNode;

			if (!(varNode = peff::makeShared<VarNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
				return genOutOfMemoryCompError();
			}

			if (!varNode->name.build(j->name))
				return genOutOfMemoryCompError();
			varNode->initialValue = j->initialValue;
			varNode->type = j->type;
			varNode->accessModifier = i->accessModifier;

			if (!mod->addMember(varNode.castTo<MemberNode>()))
				return genOutOfMemoryCompError();
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::normalizeModuleVarDefStmts(
	CompileContext *compileContext,
	peff::SharedPtr<ModuleNode> mod) {
	if (mod->isVarDefStmtsNormalized) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(renormalizeModuleVarDefStmts(compileContext, mod));

	mod->isVarDefStmtsNormalized = true;
	return {};
}
