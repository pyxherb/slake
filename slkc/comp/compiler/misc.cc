#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::reindexFnParams(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<FnOverloadingNode> fn) {
	for (size_t i = 0; i < fn->params.size(); ++i) {
		peff::SharedPtr<VarNode> &curParam = fn->params.at(i);
		if (fn->paramIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::ParamAlreadyDefined)));
		}

		if (!fn->paramIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	for (size_t i = 0; i < fn->genericParams.size(); ++i) {
		peff::SharedPtr<GenericParamNode> &curParam = fn->genericParams.at(i);
		if (fn->genericParamIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::GenericParamAlreadyDefined)));
		}

		if (!fn->genericParamIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	fn->isParamsIndexed = true;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::indexFnParams(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<FnOverloadingNode> fn) {
	if (fn->isParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexFnParams(compileEnv, fn));
	fn->isParamsIndexed = true;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::reindexClassGenericParams(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<ClassNode> cls) {
	for (size_t i = 0; i < cls->genericParams.size(); ++i) {
		peff::SharedPtr<GenericParamNode> &curParam = cls->genericParams.at(i);
		if (cls->genericParamIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::GenericParamAlreadyDefined)));
		}

		if (!cls->genericParamIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	cls->isGenericParamsIndexed = true;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::indexClassGenericParams(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<ClassNode> cls) {
	if (cls->isGenericParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexClassGenericParams(compileEnv, cls));
	cls->isGenericParamsIndexed = true;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::reindexInterfaceGenericParams(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<InterfaceNode> interfaceNode) {
	for (size_t i = 0; i < interfaceNode->genericParams.size(); ++i) {
		peff::SharedPtr<GenericParamNode> &curParam = interfaceNode->genericParams.at(i);
		if (interfaceNode->genericParamIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::GenericParamAlreadyDefined)));
		}

		if (!interfaceNode->genericParamIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	interfaceNode->isGenericParamsIndexed = true;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::indexInterfaceGenericParams(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<InterfaceNode> interfaceNode) {
	if (interfaceNode->isGenericParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexInterfaceGenericParams(compileEnv, interfaceNode));
	interfaceNode->isGenericParamsIndexed = true;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::renormalizeModuleVarDefStmts(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<ModuleNode> mod) {
	for (auto &i : mod->varDefStmts) {
		for (auto &j : i->varDefEntries) {
			if (mod->memberIndices.contains(j->name)) {
				SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(/* TODO: Use variable definition entries' token range! */ i->tokenRange, CompilationErrorKind::MemberAlreadyDefined)));
			}
			peff::SharedPtr<VarNode> varNode;

			if (!(varNode = peff::makeSharedWithControlBlock<VarNode, AstNodeControlBlock<VarNode>>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
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
	CompileEnvironment *compileEnv,
	peff::SharedPtr<ModuleNode> mod) {
	if (mod->isVarDefStmtsNormalized) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(renormalizeModuleVarDefStmts(compileEnv, mod));

	mod->isVarDefStmtsNormalized = true;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::isFnSignatureSame(
	peff::SharedPtr<VarNode> *lParams,
	peff::SharedPtr<VarNode> *rParams,
	size_t nParams,
	bool &whetherOut) {
	for (size_t i = 0; i < nParams; ++i) {
		const peff::SharedPtr<VarNode> &lCurParam = lParams[i], rCurParam = rParams[i];

		SLKC_RETURN_IF_COMP_ERROR(isSameTypeInSignature(lCurParam->type, rCurParam->type, whetherOut));

		if (!whetherOut) {
			return {};
		}
	}

	whetherOut = true;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::isFnSignatureDuplicated(peff::SharedPtr<FnOverloadingNode> lhs, peff::SharedPtr<FnOverloadingNode> rhs, bool &whetherOut) {
	if (lhs->params.size() != rhs->params.size()) {
		whetherOut = false;
		return {};
	}
	if (lhs->genericParams.size() != rhs->genericParams.size()) {
		whetherOut = false;
		return {};
	}
	return isFnSignatureSame(lhs->params.data(), rhs->params.data(), lhs->params.size(), whetherOut);
}

SLKC_API std::optional<CompilationError> slkc::indexModuleMembers(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<ModuleNode> moduleNode) {
	for (auto i : moduleNode->members) {
		switch (i->astNodeType) {
			case AstNodeType::Module: {
				peff::SharedPtr<ModuleNode> m = i.castTo<ModuleNode>();

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleMembers(compileEnv, m));
				break;
			}
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> m = i.castTo<ClassNode>();

				SLKC_RETURN_IF_COMP_ERROR(indexClassGenericParams(compileEnv, m));

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleMembers(compileEnv, m.castTo<ModuleNode>()));
				break;
			}
			case AstNodeType::Interface: {
				peff::SharedPtr<InterfaceNode> m = i.castTo<InterfaceNode>();

				SLKC_RETURN_IF_COMP_ERROR(indexInterfaceGenericParams(compileEnv, m));

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleMembers(compileEnv, m.castTo<ModuleNode>()));
				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnNode> m = i.castTo<FnNode>();

				for (auto j : m->overloadings) {
					SLKC_RETURN_IF_COMP_ERROR(indexFnParams(compileEnv, j));
				}
				break;
			}
		}
	}

	return {};
}
