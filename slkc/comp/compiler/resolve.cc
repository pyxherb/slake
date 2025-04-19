#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::getFullIdRef(peff::Alloc *allocator, peff::SharedPtr<MemberNode> m, IdRefPtr &idRefOut) {
	IdRefPtr p(peff::allocAndConstruct<IdRef>(allocator, ASTNODE_ALIGNMENT, allocator, m->document.lock()));

	for (;;) {
		IdRefEntry entry(allocator);

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

		if (!p->entries.pushBack(std::move(entry))) {
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
				peff::SharedPtr<FnSlotNode> m = result.castTo<FnSlotNode>();

				// TODO: Check if the slot contains any static method.
				memberOut = {};
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
		case AstNodeType::This: {
			peff::SharedPtr<ThisNode> cls = memberNode.castTo<ThisNode>();

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(cls->document.lock(), cls->thisType, name, memberOut));

			break;
		}
		case AstNodeType::Var: {
			peff::SharedPtr<VarNode> m = memberNode.castTo<VarNode>();

			if (m->type->typeNameKind != TypeNameKind::Custom) {
				return {};
			}

			peff::SharedPtr<TypeNameNode> type;
			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(m->type, type));

			peff::SharedPtr<MemberNode> tm;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, type.castTo<CustomTypeNameNode>(), tm));

			if (!tm) {
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(document, tm, name, result));

			break;
		}
		default:
			result = {};
	}

	if (result) {
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
				peff::SharedPtr<FnSlotNode> m = result.castTo<FnSlotNode>();

				// TODO: Check if the slot contains any instance method.
				break;
			}
			default:
				break;
		}
		memberOut = result;
		return {};
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveIdRef(
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
			SLKC_RETURN_IF_COMP_ERROR(resolveStaticMember(document, curMember, curEntry, curMember));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(document, curMember, curEntry, curMember));
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

	memberOut = curMember;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveIdRefWithScopeNode(
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
			switch (resolveScope->astNodeType) {
				case AstNodeType::Class: {
					peff::SharedPtr<ClassNode> m = resolveScope.castTo<ClassNode>();

					if (auto it = m->genericParamIndices.find(initialEntry.name); it != m->genericParamIndices.end()) {
						memberOut = m->genericParams.at(it.value()).castTo<MemberNode>();
						return {};
					}
					break;
				}
				case AstNodeType::Interface: {
					peff::SharedPtr<InterfaceNode> m = resolveScope.castTo<InterfaceNode>();

					if (auto it = m->genericParamIndices.find(initialEntry.name); it != m->genericParamIndices.end()) {
						memberOut = m->genericParams.at(it.value()).castTo<MemberNode>();
						return {};
					}
					break;
				}
				case AstNodeType::Fn: {
					peff::SharedPtr<FnNode> m = resolveScope.castTo<FnNode>();

					if (auto it = m->genericParamIndices.find(initialEntry.name); it != m->genericParamIndices.end()) {
						memberOut = m->genericParams.at(it.value()).castTo<MemberNode>();
						return {};
					}
					break;
				}
				default:;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(resolveIdRef(document, resolveScope, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

	if (!memberOut) {
		switch (resolveScope->astNodeType) {
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> m = resolveScope.castTo<ClassNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				if (m->baseType && (m->baseType->typeNameKind == TypeNameKind::Custom)) {
					peff::SharedPtr<MemberNode> baseType;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, m->baseType.castTo<CustomTypeNameNode>(), baseType, &walkedNodes));

					if (baseType && (baseType->astNodeType == AstNodeType::Class)) {
						peff::SharedPtr<ClassNode> b = baseType.castTo<ClassNode>();

						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, b.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

						if (memberOut) {
							return {};
						}
					}

					walkedNodes.clear();
				}

				for (auto &i : m->implementedTypes) {
					if (i->typeNameKind == TypeNameKind::Custom) {
						peff::SharedPtr<MemberNode> baseType;

						SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, i.castTo<CustomTypeNameNode>(), baseType, &walkedNodes));

						if (baseType && (baseType->astNodeType == AstNodeType::Interface)) {
							peff::SharedPtr<InterfaceNode> b = baseType.castTo<InterfaceNode>();

							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, b.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

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
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, p.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

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
					if (i->typeNameKind == TypeNameKind::Custom) {
						peff::SharedPtr<MemberNode> baseType;

						SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, i.castTo<CustomTypeNameNode>(), baseType, &walkedNodes));

						if (baseType && (baseType->astNodeType == AstNodeType::Interface)) {
							peff::SharedPtr<InterfaceNode> b = baseType.castTo<InterfaceNode>();

							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, b.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

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
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, p, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

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
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, p, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

						if (memberOut) {
							return {};
						}
					}
				}
				break;
			}
			case AstNodeType::Fn: {
				peff::SharedPtr<FnNode> m = resolveScope.castTo<FnNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				if (!m->parent)
					std::terminate();

				peff::SharedPtr<FnSlotNode> slot;
				{
					peff::SharedPtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					if (p->astNodeType != AstNodeType::FnSlot)
						std::terminate();
					slot = p.castTo<FnSlotNode>();
				}

				if (slot->parent) {
					SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, walkedNodes, slot->parent->sharedFromThis().castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, false));
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
		SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, *walkedNodes, typeName->contextNode.lock(), typeName->idRefPtr->entries.data(), typeName->idRefPtr->entries.size(), member, nullptr));
	} else {
		peff::Set<peff::SharedPtr<MemberNode>> myWalkedNodes(document->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(document, myWalkedNodes, typeName->contextNode.lock(), typeName->idRefPtr->entries.data(), typeName->idRefPtr->entries.size(), member, nullptr));
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
