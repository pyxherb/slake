#include "../compiler.h"

using namespace slkc;

static peff::Option<CompilationError> _determineWithCurrentSlot(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnNode> fnSlot,
	const AstNodePtr<TypeNameNode> *argTypes,
	size_t nArgTypes,
	bool isStatic,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matchedOverloadings,
	peff::Set<AstNodePtr<MemberNode>> *walkedParents) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 8));

	for (size_t i = 0; i < fnSlot->overloadings.size(); ++i) {
		bool exactlyMatched = true;
		AstNodePtr<FnOverloadingNode> currentOverloading = fnSlot->overloadings.at(i);

		if (isStatic != ((currentOverloading->accessModifier & slake::ACCESS_STATIC) == slake::ACCESS_STATIC)) {
			continue;
		}

		if (nArgTypes < currentOverloading->params.size()) {
			continue;
		} else if (nArgTypes > currentOverloading->params.size()) {
			if (!(currentOverloading->fnFlags & FN_VARG)) {
				continue;
			}
		}

		for (size_t j = 0; j < currentOverloading->params.size(); ++j) {
			AstNodePtr<VarNode> currentParam = currentOverloading->params.at(j);

			bool whether = false;
			SLKC_RETURN_IF_COMP_ERROR(isSameType(currentParam->type, argTypes[j], whether));

			if (!whether) {
				SLKC_RETURN_IF_COMP_ERROR(isConvertible(argTypes[j], currentParam->type, true, whether));
				if (!whether) {
					goto mismatched;
				}
				exactlyMatched = false;
			}
		}

		if (exactlyMatched) {
			matchedOverloadings.clear();

			if (!matchedOverloadings.pushBack(AstNodePtr<FnOverloadingNode>(currentOverloading))) {
				return genOutOfMemoryCompError();
			}

			return {};
		} else {
			if (!matchedOverloadings.pushBack(AstNodePtr<FnOverloadingNode>(currentOverloading))) {
				return genOutOfMemoryCompError();
			}
		}

	mismatched:;
	}
	return {};
}

static peff::Option<CompilationError> _determineWithParentClass(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnNode> fnSlot,
	const AstNodePtr<TypeNameNode> *argTypes,
	size_t nArgTypes,
	bool isStatic,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matchedOverloadings,
	peff::Set<AstNodePtr<MemberNode>> *walkedParents) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 8));

	AstNodePtr<ClassNode> m = fnSlot->parent->sharedFromThis().castTo<ClassNode>();
	{
		AstNodePtr<ClassNode> baseType;
		SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->baseType, baseType, nullptr));
		if (baseType) {
			if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
				if (baseType->members.at(it.value())->getAstNodeType() != AstNodeType::Fn) {
					goto classBaseClassMalformed;
				}

				if (!walkedParents) {
					peff::Set<AstNodePtr<MemberNode>> walkedParentsSet(compileEnv->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, baseType->members.at(it.value()).castTo<FnNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, baseType->members.at(it.value()).castTo<FnNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
				}
			}
		}
	}

classBaseClassMalformed:
	for (auto &i : m->implTypes) {
		{
			AstNodePtr<InterfaceNode> baseType;
			SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
			if (baseType) {
				if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
					if (baseType->members.at(it.value())->getAstNodeType() != AstNodeType::Fn) {
						continue;
					}

					if (!walkedParents) {
						peff::Set<AstNodePtr<MemberNode>> walkedParentsSet(compileEnv->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, baseType->members.at(it.value()).castTo<FnNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, baseType->members.at(it.value()).castTo<FnNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
					}
				}
			}
		}
	}
	return {};
}

static peff::Option<CompilationError> _determineWithParentInterface(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnNode> fnSlot,
	const AstNodePtr<TypeNameNode> *argTypes,
	size_t nArgTypes,
	bool isStatic,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matchedOverloadings,
	peff::Set<AstNodePtr<MemberNode>> *walkedParents) {
	AstNodePtr<InterfaceNode> m = fnSlot->parent->sharedFromThis().castTo<InterfaceNode>();
	for (auto &i : m->implTypes) {
		{
			AstNodePtr<InterfaceNode> baseType;
			SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
			if (baseType) {
				if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
					if (baseType->members.at(it.value())->getAstNodeType() != AstNodeType::Fn) {
						continue;
					}

					if (!walkedParents) {
						peff::Set<AstNodePtr<MemberNode>> walkedParentsSet(compileEnv->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, baseType->members.at(it.value()).castTo<FnNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, baseType->members.at(it.value()).castTo<FnNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
					}
				}
			}
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::determineFnOverloading(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnNode> fnSlot,
	const AstNodePtr<TypeNameNode> *argTypes,
	size_t nArgTypes,
	bool isStatic,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matchedOverloadings,
	peff::Set<AstNodePtr<MemberNode>> *walkedParents) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 1));

	SLKC_RETURN_IF_COMP_ERROR(_determineWithCurrentSlot(compileEnv, fnSlot, argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));

	if (isStatic) {
		// ...
	} else
		switch (fnSlot->parent->getAstNodeType()) {
			case AstNodeType::Class:
				SLKC_RETURN_IF_COMP_ERROR(_determineWithParentClass(compileEnv, fnSlot, argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
				break;
			case AstNodeType::Interface:
				SLKC_RETURN_IF_COMP_ERROR(_determineWithParentInterface(compileEnv, fnSlot, argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
				break;
		}

	return {};
}
