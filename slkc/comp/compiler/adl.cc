#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::determineFnOverloading(
	CompileContext *compileContext,
	peff::SharedPtr<FnSlotNode> fnSlot,
	const peff::SharedPtr<TypeNameNode> *argTypes,
	size_t nArgTypes,
	bool isStatic,
	peff::DynArray<peff::SharedPtr<FnNode>> &matchedOverloadings,
	peff::Set<peff::SharedPtr<MemberNode>> *walkedParents) {
	for (size_t i = 0; i < fnSlot->overloadings.size(); ++i) {
		bool exactlyMatched = true;
		peff::SharedPtr<FnNode> currentOverloading = fnSlot->overloadings.at(i);

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
			peff::SharedPtr<VarNode> currentParam = currentOverloading->params.at(j);

			bool whether = false;
			SLKC_RETURN_IF_COMP_ERROR(isSameType(currentParam->type, argTypes[j], whether));

			if (!whether) {
				SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(argTypes[j], currentParam->type, whether));
				if (!whether) {
					goto mismatched;
				}
				exactlyMatched = false;
			}
		}

		if (exactlyMatched) {
			matchedOverloadings.clear();

			if (!matchedOverloadings.pushBack(peff::SharedPtr<FnNode>(currentOverloading))) {
				return genOutOfMemoryCompError();
			}

			return {};
		} else {
			if (!matchedOverloadings.pushBack(peff::SharedPtr<FnNode>(currentOverloading))) {
				return genOutOfMemoryCompError();
			}
		}

		mismatched:;
	}

	switch (fnSlot->parent->astNodeType) {
		case AstNodeType::Module: {
			if (fnSlot->parent->parent) {
				peff::SharedPtr<ModuleNode> baseType = fnSlot->parent->parent->sharedFromThis().castTo<ModuleNode>();

				if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
					if (baseType->members.at(it.value())->astNodeType != AstNodeType::FnSlot) {
						goto baseModuleMalformed;
					}

					if (!walkedParents) {
						peff::Set<peff::SharedPtr<MemberNode>> walkedParentsSet(compileContext->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
					}
				}
			}

		baseModuleMalformed:
			break;
		}
		case AstNodeType::Class: {
			peff::SharedPtr<ClassNode> m = fnSlot->parent->sharedFromThis().castTo<ClassNode>();
			{
				peff::SharedPtr<ClassNode> baseType;
				SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->baseType, baseType, nullptr));
				if (baseType) {
					if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
						if (baseType->members.at(it.value())->astNodeType != AstNodeType::FnSlot) {
							goto classBaseClassMalformed;
						}

						if (!walkedParents) {
							peff::Set<peff::SharedPtr<MemberNode>> walkedParentsSet(compileContext->allocator.get());
							SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
						}
					}
				}
			}

		classBaseClassMalformed:
			for (auto &i : m->implementedTypes) {
				{
					peff::SharedPtr<InterfaceNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
					if (baseType) {
						if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
							if (baseType->members.at(it.value())->astNodeType != AstNodeType::FnSlot) {
								continue;
							}

							if (!walkedParents) {
								peff::Set<peff::SharedPtr<MemberNode>> walkedParentsSet(compileContext->allocator.get());
								SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
							} else {
								SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
							}
						}
					}
				}
			}
			break;
		}
		case AstNodeType::Interface: {
			peff::SharedPtr<InterfaceNode> m = fnSlot->parent->sharedFromThis().castTo<InterfaceNode>();
			for (auto &i : m->implementedTypes) {
				{
					peff::SharedPtr<InterfaceNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
					if (baseType) {
						if (auto it = baseType->memberIndices.find(fnSlot->name); it != baseType->memberIndices.end()) {
							if (baseType->members.at(it.value())->astNodeType != AstNodeType::FnSlot) {
								continue;
							}

							if (!walkedParents) {
								peff::Set<peff::SharedPtr<MemberNode>> walkedParentsSet(compileContext->allocator.get());
								SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings, walkedParents));
							} else {
								SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, baseType->members.at(it.value()).castTo<FnSlotNode>(), argTypes, nArgTypes, isStatic, matchedOverloadings));
							}
						}
					}
				}
			}
			break;
		}
	}

	return {};
}
