#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::getFullIdRef(peff::Alloc *allocator, peff::SharedPtr<MemberNode> m, IdRefPtr &idRefOut) {
	IdRefPtr p(peff::allocAndConstruct<IdRef>(allocator, ASTNODE_ALIGNMENT, allocator));

	for (;;) {
		IdRefEntry entry(allocator);

		if (!m->name.size()) {
			break;
		}

		if (!entry.name.build(m->name)) {
			return genOutOfMemoryCompError();
		}

		if (!entry.genericArgs.resize(m->genericArgs.size())) {
			return genOutOfMemoryCompError();
		}

		for (size_t i = 0; i < entry.genericArgs.size(); ++i) {
			if (!(entry.genericArgs.at(i) = m->genericArgs.at(i)->duplicate<TypeNameNode>(allocator))) {
				return genOutOfMemoryCompError();
			}
		}

		if (!p->entries.pushFront(std::move(entry))) {
			return genOutOfMemoryCompError();
		}

		if (!m->parent)
			break;

		m = m->parent->sharedFromThis().castTo<MemberNode>();
	}

	idRefOut = std::move(p);

	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveStaticMember(
	CompileContext *compileContext,
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<MemberNode> &memberNode,
	const IdRefEntry &name,
	peff::SharedPtr<MemberNode> &memberOut) {
	peff::SharedPtr<MemberNode> result;

	switch (memberNode->astNodeType) {
		case AstNodeType::Module: {
			peff::SharedPtr<ModuleNode> mod = memberNode.castTo<ModuleNode>();

			if (auto it = mod->memberIndices.find(name.name); it != mod->memberIndices.end()) {
				result = mod->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Class: {
			peff::SharedPtr<ClassNode> cls = memberNode.castTo<ClassNode>();

			if (auto it = cls->memberIndices.find(name.name); it != cls->memberIndices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Interface: {
			peff::SharedPtr<InterfaceNode> cls = memberNode.castTo<InterfaceNode>();

			if (auto it = cls->memberIndices.find(name.name); it != cls->memberIndices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		default:
			result = {};
	}

	if (result) {
		if (name.genericArgs.size()) {
			SLKC_RETURN_IF_COMP_ERROR(document->instantiateGenericObject(result, name.genericArgs, result));
		}

		switch (result->astNodeType) {
			case AstNodeType::Var: {
				peff::SharedPtr<VarNode> m = result.castTo<VarNode>();

				// Check if the variable member is static.
				if (!(m->accessModifier & slake::ACCESS_STATIC)) {
					memberOut = {};
					return {};
				}
				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnNode> m = result.castTo<FnNode>();

				// Check if the slot contains any static method.
				for (auto i : m->overloadings) {
					if (i->accessModifier & slake::ACCESS_STATIC)
						goto pass;
				}

				memberOut = {};
				return {};
			pass:
				break;
			}
			default:
				break;
		}
		memberOut = result;
		return {};
	}

	memberOut = {};
	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveInstanceMember(
	CompileContext *compileContext,
	peff::SharedPtr<Document> document,
	peff::SharedPtr<MemberNode> memberNode,
	const IdRefEntry &name,
	peff::SharedPtr<MemberNode> &memberOut) {
	peff::SharedPtr<MemberNode> result;

	switch (memberNode->astNodeType) {
		case AstNodeType::Module: {
			peff::SharedPtr<ModuleNode> mod = memberNode.castTo<ModuleNode>();

			if (auto it = mod->memberIndices.find(name.name); it != mod->memberIndices.end()) {
				result = mod->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Class: {
			peff::SharedPtr<ClassNode> m = memberNode.castTo<ClassNode>();

			if (auto it = m->memberIndices.find(name.name); it != m->memberIndices.end()) {
				result = m->members.at(it.value());
			} else {
				{
					peff::SharedPtr<ClassNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->baseType, baseType, nullptr));
					if (baseType) {
						SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, document, baseType.castTo<MemberNode>(), name, result));

						if (result) {
							goto classResolutionSucceeded;
						}
					}
				}

				for (auto &i : m->implementedTypes) {
					{
						peff::SharedPtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, document, baseType.castTo<MemberNode>(), name, result));

							if (result) {
								goto classResolutionSucceeded;
							}
						}
					}
				}
			}

		classResolutionSucceeded:
			break;
		}
		case AstNodeType::Interface: {
			peff::SharedPtr<InterfaceNode> m = memberNode.castTo<InterfaceNode>();

			if (auto it = m->memberIndices.find(name.name); it != m->memberIndices.end()) {
				result = m->members.at(it.value());
			} else {
				for (auto &i : m->implementedTypes) {
					{
						peff::SharedPtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, document, baseType.castTo<MemberNode>(), name, result));

							if (result) {
								goto interfaceResolutionSucceeded;
							}
						}
					}
				}
			}

		interfaceResolutionSucceeded:
			break;
		}
		case AstNodeType::This: {
			peff::SharedPtr<ThisNode> cls = memberNode.castTo<ThisNode>();

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, cls->document.lock(), cls->thisType, name, result));

			break;
		}
		case AstNodeType::Var: {
			peff::SharedPtr<VarNode> m = memberNode.castTo<VarNode>();

			if (m->type->typeNameKind != TypeNameKind::Custom) {
				result = {};
				break;
			}

			peff::SharedPtr<TypeNameNode> type;
			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(m->type, type));

			peff::SharedPtr<MemberNode> tm;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, type.castTo<CustomTypeNameNode>(), tm));

			if (!tm) {
				result = {};
				break;
			}

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, document, tm, name, result));

			break;
		}
		default:
			result = {};
	}

	if (result) {
		if (name.genericArgs.size()) {
			SLKC_RETURN_IF_COMP_ERROR(document->instantiateGenericObject(result, name.genericArgs, result));
		}

		switch (result->astNodeType) {
			case AstNodeType::Var: {
				peff::SharedPtr<VarNode> m = result.castTo<VarNode>();

				// Check if the variable member is static or not.
				if (m->accessModifier & slake::ACCESS_STATIC) {
					return {};
				}
				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnNode> m = result.castTo<FnNode>();

				// Check if the slot contains any instance method.
				for (auto i : m->overloadings) {
					if (!(i->accessModifier & slake::ACCESS_STATIC))
						goto pass;
				}

				memberOut = {};
				return {};
			pass:
				break;
			}
			default:
				break;
		}
		memberOut = result;
		return {};
	}

	memberOut = {};
	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveIdRef(
	CompileContext *compileContext,
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<MemberNode> &resolveRoot,
	IdRefEntry *idRef,
	size_t nEntries,
	peff::SharedPtr<MemberNode> &memberOut,
	ResolvedIdRefPartList *resolvedPartListOut,
	bool isStatic) {
	peff::SharedPtr<MemberNode> curMember = resolveRoot;

	bool isPostStaticParts = false;

	auto updateStaticStatus = [&curMember, &isStatic]() {
		switch (curMember->astNodeType) {
			case AstNodeType::Var:
				isStatic = false;
				break;
			default:
				break;
		}
	};

	updateStaticStatus();

	for (size_t i = 0; i < nEntries; ++i) {
		const IdRefEntry &curEntry = idRef[i];
		if (isStatic) {
			SLKC_RETURN_IF_COMP_ERROR(resolveStaticMember(compileContext, document, curMember, curEntry, curMember));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, document, curMember, curEntry, curMember));
		}

		if (!curMember) {
			memberOut = {};
			if (resolvedPartListOut) {
				resolvedPartListOut->clear();
			}
			return {};
		}

		updateStaticStatus();

		// We assume that all parts after the static parts are in instance.
		if (resolvedPartListOut) {
			if (!isStatic) {
				if (!isPostStaticParts) {
					ResolvedIdRefPart part = { isStatic, i, curMember };

					if (!resolvedPartListOut->pushBack(std::move(part)))
						return genOutOfMemoryCompError();

					isPostStaticParts = true;
				} else {
					ResolvedIdRefPart part = { isStatic, 1, curMember };

					if (!resolvedPartListOut->pushBack(std::move(part)))
						return genOutOfMemoryCompError();
				}
			}
		}
	}

	if (resolvedPartListOut) {
		if (isStatic) {
			ResolvedIdRefPart part = { isStatic, nEntries, curMember };

			if (!resolvedPartListOut->pushBack(std::move(part)))
				return genOutOfMemoryCompError();
		}
	}

	memberOut = curMember;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveIdRefWithScopeNode(
	CompileContext *compileContext,
	peff::SharedPtr<Document> document,
	peff::Set<peff::SharedPtr<MemberNode>> &walkedNodes,
	const peff::SharedPtr<MemberNode> &resolveScope,
	IdRefEntry *idRef,
	size_t nEntries,
	peff::SharedPtr<MemberNode> &memberOut,
	ResolvedIdRefPartList *resolvedPartListOut,
	bool isStatic,
	bool isSealed) {
	if (walkedNodes.contains(resolveScope)) {
		memberOut = {};
		return {};
	}

	if ((nEntries == 1) && (!isSealed)) {
		const IdRefEntry &initialEntry = idRef[0];

		if (!initialEntry.genericArgs.size()) {
			peff::SharedPtr<MemberNode> curScope = resolveScope;

		reresolveWithNewScope:
			switch (curScope->astNodeType) {
				case AstNodeType::Class: {
					peff::SharedPtr<ClassNode> m = curScope.castTo<ClassNode>();

					if (auto it = m->genericParamIndices.find(initialEntry.name); it != m->genericParamIndices.end()) {
						memberOut = m->genericParams.at(it.value()).castTo<MemberNode>();
						return {};
					}
					if (m->parent) {
						curScope = m->parent->sharedFromThis().castTo<MemberNode>();
						goto reresolveWithNewScope;
					}
					break;
				}
				case AstNodeType::Interface: {
					peff::SharedPtr<InterfaceNode> m = curScope.castTo<InterfaceNode>();

					if (auto it = m->genericParamIndices.find(initialEntry.name); it != m->genericParamIndices.end()) {
						memberOut = m->genericParams.at(it.value()).castTo<MemberNode>();
						return {};
					}
					if (m->parent) {
						curScope = m->parent->sharedFromThis().castTo<MemberNode>();
						goto reresolveWithNewScope;
					}
					break;
				}
				case AstNodeType::Fn: {
					peff::SharedPtr<FnOverloadingNode> m = curScope.castTo<FnOverloadingNode>();

					if (auto it = m->genericParamIndices.find(initialEntry.name); it != m->genericParamIndices.end()) {
						memberOut = m->genericParams.at(it.value()).castTo<MemberNode>();
						return {};
					}

					curScope = m->parent->parent->sharedFromThis().castTo<MemberNode>();
					goto reresolveWithNewScope;
				}
				default:;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(resolveIdRef(compileContext, document, resolveScope, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

	if (!memberOut) {
		switch (resolveScope->astNodeType) {
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> m = resolveScope.castTo<ClassNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				{
					peff::SharedPtr<ClassNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->baseType, baseType, &walkedNodes));
					if (baseType) {
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, baseType.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

						if (memberOut) {
							return {};
						}
					}
				}
				walkedNodes.clear();

				for (auto &i : m->implementedTypes) {
					{
						peff::SharedPtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, &walkedNodes));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, baseType.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

							if (memberOut) {
								return {};
							}
						}
					}
					walkedNodes.clear();
				}

				if (m->parent && (!isSealed)) {
					peff::SharedPtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					if (p->astNodeType == AstNodeType::Module) {
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, p.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

						if (memberOut) {
							return {};
						}
					}
				}
				break;
			}
			case AstNodeType::Interface: {
				peff::SharedPtr<InterfaceNode> m = resolveScope.castTo<InterfaceNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				for (auto &i : m->implementedTypes) {
					{
						peff::SharedPtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, &walkedNodes));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, baseType.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

							if (memberOut) {
								return {};
							}
						}
					}
					walkedNodes.clear();
				}

				if (m->parent && (!isSealed)) {
					peff::SharedPtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					if (p->astNodeType == AstNodeType::Module) {
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, p, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

						if (memberOut) {
							return {};
						}
					}
				}
				break;
			}
			case AstNodeType::Module: {
				peff::SharedPtr<ModuleNode> m = resolveScope.castTo<ModuleNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				if (m->parent) {
					peff::SharedPtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					if (p->astNodeType == AstNodeType::Module) {
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, p, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

						if (memberOut) {
							return {};
						}
					}
				}
				break;
			}
			case AstNodeType::Fn: {
				peff::SharedPtr<FnOverloadingNode> m = resolveScope.castTo<FnOverloadingNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				if (!m->parent)
					std::terminate();

				peff::SharedPtr<FnNode> slot;
				{
					peff::SharedPtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					if (p->astNodeType != AstNodeType::FnSlot)
						std::terminate();
					slot = p.castTo<FnNode>();
				}

				if (slot->parent) {
					SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileContext, document, walkedNodes, slot->parent->sharedFromThis().castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, false));
				}

				if (memberOut) {
					return {};
				}
				break;
			}
			default:
				break;
		}
	} else {
		return {};
	}

	memberOut = {};
	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveCustomTypeName(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<CustomTypeNameNode> &typeName,
	peff::SharedPtr<MemberNode> &memberNodeOut,
	peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes) {
	peff::SharedPtr<MemberNode> member;

	if (typeName->cachedResolveResult.isValid()) {
		member = typeName->cachedResolveResult.lock();
		goto resolved;
	}

	if (walkedNodes) {
		SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(nullptr, document, *walkedNodes, typeName->contextNode.lock(), typeName->idRefPtr->entries.data(), typeName->idRefPtr->entries.size(), member, nullptr));
	} else {
		peff::Set<peff::SharedPtr<MemberNode>> myWalkedNodes(document->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(nullptr, document, myWalkedNodes, typeName->contextNode.lock(), typeName->idRefPtr->entries.data(), typeName->idRefPtr->entries.size(), member, nullptr));
	}

	if (member) {
		goto resolved;
	}

resolved:
	if (member) {
		typeName->cachedResolveResult = peff::WeakPtr<MemberNode>(member);

		switch (member->astNodeType) {
			case AstNodeType::Class:
			case AstNodeType::Interface:
			case AstNodeType::GenericParam:
				memberNodeOut = member;
				return {};
			default:;
		}

		memberNodeOut = {};
		return {};
	}

	memberNodeOut = {};
	return {};
}

SLKC_API std::optional<CompilationError> slkc::visitBaseClass(peff::SharedPtr<TypeNameNode> cls, peff::SharedPtr<ClassNode> &classOut, peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes) {
	if (cls && (cls->typeNameKind == TypeNameKind::Custom)) {
		peff::SharedPtr<MemberNode> baseType;

		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(cls->document.lock(), cls.castTo<CustomTypeNameNode>(), baseType, walkedNodes));

		if (baseType && (baseType->astNodeType == AstNodeType::Class)) {
			peff::SharedPtr<ClassNode> b = baseType.castTo<ClassNode>();

			if ((!walkedNodes) || !walkedNodes->contains(baseType)) {
				classOut = b;
			}
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::visitBaseInterface(peff::SharedPtr<TypeNameNode> cls, peff::SharedPtr<InterfaceNode> &classOut, peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes) {
	if (cls && (cls->typeNameKind == TypeNameKind::Custom)) {
		peff::SharedPtr<MemberNode> baseType;

		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(cls->document.lock(), cls.castTo<CustomTypeNameNode>(), baseType, walkedNodes));

		if (baseType && (baseType->astNodeType == AstNodeType::Interface)) {
			peff::SharedPtr<InterfaceNode> b = baseType.castTo<InterfaceNode>();

			if (walkedNodes && !walkedNodes->contains(baseType)) {
				classOut = b;
			}
		}
	}

	return {};
}
