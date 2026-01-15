#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::getSucceedingEnumValue(
	CompileEnvironment* compileEnv,
	CompilationContext* compilationContext,
	AstNodePtr<TypeNameNode> baseType,
	AstNodePtr<ExprNode> lastValue,
	AstNodePtr<ExprNode>& valueOut) {
	if (lastValue) {
		switch (baseType->typeNameKind) {
			case TypeNameKind::I8:
				if (!(valueOut = makeAstNode<I8LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<I8LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::I16:
				if (!(valueOut = makeAstNode<I16LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<I16LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::I32:
				if (!(valueOut = makeAstNode<I32LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<I32LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::I64:
				if (!(valueOut = makeAstNode<I64LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<I64LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U8:
				if (!(valueOut = makeAstNode<U8LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<U8LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U16:
				if (!(valueOut = makeAstNode<U16LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<U16LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U32:
				if (!(valueOut = makeAstNode<U32LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<U32LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U64:
				if (!(valueOut = makeAstNode<U8LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<U64LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::F32:
				if (!(valueOut = makeAstNode<F32LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<F32LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::F64:
				if (!(valueOut = makeAstNode<F64LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  lastValue.castTo<F64LiteralExprNode>()->data + 1)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::Bool:
				if (!(valueOut = makeAstNode<BoolLiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  !lastValue.castTo<BoolLiteralExprNode>()->data)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			default:
				std::terminate();
		}
	} else {
		switch (baseType->typeNameKind) {
			case TypeNameKind::I8:
				if (!(valueOut = makeAstNode<I8LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::I16:
				if (!(valueOut = makeAstNode<I16LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::I32:
				if (!(valueOut = makeAstNode<I32LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::I64:
				if (!(valueOut = makeAstNode<I64LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U8:
				if (!(valueOut = makeAstNode<U8LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U16:
				if (!(valueOut = makeAstNode<U16LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U32:
				if (!(valueOut = makeAstNode<U32LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::U64:
				if (!(valueOut = makeAstNode<U8LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::F32:
				if (!(valueOut = makeAstNode<F32LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::F64:
				if (!(valueOut = makeAstNode<F64LiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  0)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			case TypeNameKind::Bool:
				if (!(valueOut = makeAstNode<BoolLiteralExprNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document,
						  false)
							.castTo<ExprNode>()))
					return genOutOfMemoryCompError();
				break;
			default:
				std::terminate();
		}
	}
}

SLKC_API peff::Option<CompilationError> slkc::fillScopedEnum(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ScopedEnumNode> enumNode) {
	if (!enumNode->baseType)
		return {};

	AstNodePtr<ExprNode> lastValue;

	for (size_t i = 0; i < enumNode->members.size(); ++i) {
		assert(enumNode->getAstNodeType() == AstNodeType::EnumItem);

		AstNodePtr<EnumItemNode> item = enumNode->members.at(i).castTo<EnumItemNode>();
		AstNodePtr<ExprNode> fillValue;

		if (item->enumValue) {
			bool isSame;

			AstNodePtr<TypeNameNode> tn;

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, item->enumValue, fillValue));
			if (!fillValue)
				return CompilationError(item->enumValue->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, fillValue, tn, enumNode->baseType));

			SLKC_RETURN_IF_COMP_ERROR(isSameType(tn, enumNode->baseType, isSame));

			if (!isSame) {
				AstNodePtr<CastExprNode> castExpr;

				if (!(castExpr = makeAstNode<CastExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
					return genOutOfMemoryCompError();

				castExpr->tokenRange = item->enumValue->tokenRange;
				castExpr->source = fillValue;
				castExpr->targetType = enumNode->baseType;

				SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, castExpr.castTo<ExprNode>(), fillValue));

				if (!fillValue)
					return CompilationError(item->enumValue->tokenRange, CompilationErrorKind::IncompatibleInitialValueType);
			}
		} else {
			SLKC_RETURN_IF_COMP_ERROR(getSucceedingEnumValue(compileEnv, compilationContext, enumNode->baseType, lastValue, fillValue));
		}
		lastValue = (item->filledValue = fillValue);
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::reindexFnParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnOverloadingNode> fn) {
	for (size_t i = 0; i < fn->params.size(); ++i) {
		AstNodePtr<VarNode> &curParam = fn->params.at(i);
		if (fn->paramIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::ParamAlreadyDefined)));
		}

		if (!fn->paramIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	for (size_t i = 0; i < fn->genericParams.size(); ++i) {
		AstNodePtr<GenericParamNode> &curParam = fn->genericParams.at(i);
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

SLKC_API peff::Option<CompilationError> slkc::indexFnParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnOverloadingNode> fn) {
	if (fn->isParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexFnParams(compileEnv, fn));
	fn->isParamsIndexed = true;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::reindexClassGenericParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<ClassNode> cls) {
	for (size_t i = 0; i < cls->genericParams.size(); ++i) {
		AstNodePtr<GenericParamNode> &curParam = cls->genericParams.at(i);
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

SLKC_API peff::Option<CompilationError> slkc::indexClassGenericParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<ClassNode> cls) {
	if (cls->isGenericParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexClassGenericParams(compileEnv, cls));
	cls->isGenericParamsIndexed = true;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::reindexInterfaceGenericParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<InterfaceNode> interfaceNode) {
	for (size_t i = 0; i < interfaceNode->genericParams.size(); ++i) {
		AstNodePtr<GenericParamNode> &curParam = interfaceNode->genericParams.at(i);
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

SLKC_API peff::Option<CompilationError> slkc::indexInterfaceGenericParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<InterfaceNode> interfaceNode) {
	if (interfaceNode->isGenericParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexInterfaceGenericParams(compileEnv, interfaceNode));
	interfaceNode->isGenericParamsIndexed = true;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::reindexStructGenericParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<StructNode> structNode) {
	for (size_t i = 0; i < structNode->genericParams.size(); ++i) {
		AstNodePtr<GenericParamNode> &curParam = structNode->genericParams.at(i);
		if (structNode->genericParamIndices.contains(curParam->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(curParam->tokenRange, CompilationErrorKind::GenericParamAlreadyDefined)));
		}

		if (!structNode->genericParamIndices.insert(curParam->name, +i)) {
			return genOutOfMemoryCompError();
		}
	}

	structNode->isGenericParamsIndexed = true;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::indexStructGenericParams(
	CompileEnvironment *compileEnv,
	AstNodePtr<StructNode> structNode) {
	if (structNode->isGenericParamsIndexed) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(reindexStructGenericParams(compileEnv, structNode));
	structNode->isGenericParamsIndexed = true;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::renormalizeModuleVarDefStmts(
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> mod) {
	for (auto &i : mod->varDefStmts) {
		for (auto &j : i->varDefEntries) {
			if (mod->memberIndices.contains(j->name)) {
				SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(/* TODO: Use variable definition entries' token range! */ i->tokenRange, CompilationErrorKind::MemberAlreadyDefined)));
			}
			AstNodePtr<VarNode> varNode;

			if (!(varNode = makeAstNode<VarNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
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

SLKC_API peff::Option<CompilationError> slkc::normalizeModuleVarDefStmts(
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> mod) {
	if (mod->isVarDefStmtsNormalized) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(renormalizeModuleVarDefStmts(compileEnv, mod));

	mod->isVarDefStmtsNormalized = true;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isFnSignatureSame(
	AstNodePtr<VarNode> *lParams,
	AstNodePtr<VarNode> *rParams,
	size_t nParams,
	AstNodePtr<TypeNameNode> lOverridenType,
	AstNodePtr<TypeNameNode> rOverridenType,
	bool &whetherOut) {
	if (lOverridenType || rOverridenType) {
		if ((!lOverridenType) || (!rOverridenType)) {
			whetherOut = false;
			return {};
		}
		SLKC_RETURN_IF_COMP_ERROR(isSameType(lOverridenType, rOverridenType, whetherOut));
		if (!whetherOut)
			return {};
	}

	for (size_t i = 0; i < nParams; ++i) {
		const AstNodePtr<VarNode> &lCurParam = lParams[i], rCurParam = rParams[i];

		SLKC_RETURN_IF_COMP_ERROR(isSameTypeInSignature(lCurParam->type, rCurParam->type, whetherOut));

		if (!whetherOut) {
			return {};
		}
	}

	whetherOut = true;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isFnSignatureDuplicated(AstNodePtr<FnOverloadingNode> lhs, AstNodePtr<FnOverloadingNode> rhs, bool &whetherOut) {
	if (lhs->params.size() != rhs->params.size()) {
		whetherOut = false;
		return {};
	}
	if (lhs->genericParams.size() != rhs->genericParams.size()) {
		whetherOut = false;
		return {};
	}
	return isFnSignatureSame(lhs->params.data(), rhs->params.data(), lhs->params.size(), lhs->overridenType, rhs->overridenType, whetherOut);
}

SLKC_API peff::Option<CompilationError> slkc::indexModuleVarMembers(
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> moduleNode) {
	for (auto i : moduleNode->members) {
		switch (i->getAstNodeType()) {
			case AstNodeType::Module: {
				AstNodePtr<ModuleNode> m = i.castTo<ModuleNode>();

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleVarMembers(compileEnv, m));
				break;
			}
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> m = i.castTo<ClassNode>();

				SLKC_RETURN_IF_COMP_ERROR(indexClassGenericParams(compileEnv, m));

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleVarMembers(compileEnv, m.castTo<ModuleNode>()));
				break;
			}
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> m = i.castTo<InterfaceNode>();

				SLKC_RETURN_IF_COMP_ERROR(indexInterfaceGenericParams(compileEnv, m));

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleVarMembers(compileEnv, m.castTo<ModuleNode>()));
				break;
			}
			case AstNodeType::Struct: {
				AstNodePtr<StructNode> m = i.castTo<StructNode>();

				SLKC_RETURN_IF_COMP_ERROR(indexStructGenericParams(compileEnv, m));

				SLKC_RETURN_IF_COMP_ERROR(normalizeModuleVarDefStmts(compileEnv, m.castTo<ModuleNode>()));

				SLKC_RETURN_IF_COMP_ERROR(indexModuleVarMembers(compileEnv, m.castTo<ModuleNode>()));
				break;
			}
			case AstNodeType::Fn: {
				AstNodePtr<FnNode> m = i.castTo<FnNode>();

				for (auto j : m->overloadings) {
					SLKC_RETURN_IF_COMP_ERROR(indexFnParams(compileEnv, j));
				}
				break;
			}
		}
	}

	return {};
}
