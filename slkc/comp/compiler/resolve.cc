#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::getFullIdRef(peff::Alloc *allocator, AstNodePtr<MemberNode> m, IdRefPtr &idRefOut) {
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

		switch (m->astNodeType) {
			case AstNodeType::Fn:
				if (!m->parent->parent)
					goto end;
				m = m->parent->parent->sharedFromThis().castTo<MemberNode>();
				break;
			default:
				if (!m->parent)
					goto end;
				m = m->parent->sharedFromThis().castTo<MemberNode>();
		}
	}

end:
	idRefOut = std::move(p);

	return {};
}

SLKC_API std::optional<CompilationError> slkc::resolveStaticMember(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<Document> document,
	const AstNodePtr<MemberNode> &memberNode,
	const IdRefEntry &name,
	AstNodePtr<MemberNode> &memberOut) {
	AstNodePtr<MemberNode> result;

	switch (memberNode->astNodeType) {
		case AstNodeType::Module: {
			AstNodePtr<ModuleNode> mod = memberNode.castTo<ModuleNode>();

			if (auto it = mod->memberIndices.find(name.name); it != mod->memberIndices.end()) {
				result = mod->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Class: {
			AstNodePtr<ClassNode> cls = memberNode.castTo<ClassNode>();

			if (auto it = cls->memberIndices.find(name.name); it != cls->memberIndices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Interface: {
			AstNodePtr<InterfaceNode> cls = memberNode.castTo<InterfaceNode>();

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
				AstNodePtr<VarNode> m = result.castTo<VarNode>();

				// Check if the variable member is static.
				if (!(m->accessModifier & slake::ACCESS_STATIC)) {
					memberOut = {};
					return {};
				}
				break;
			}
			case AstNodeType::FnSlot: {
				AstNodePtr<FnNode> m = result.castTo<FnNode>();

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
	CompileEnvironment *compileEnv,
	peff::SharedPtr<Document> document,
	AstNodePtr<MemberNode> memberNode,
	const IdRefEntry &name,
	AstNodePtr<MemberNode> &memberOut) {
	AstNodePtr<MemberNode> result;

	switch (memberNode->astNodeType) {
		case AstNodeType::Module: {
			AstNodePtr<ModuleNode> mod = memberNode.castTo<ModuleNode>();

			if (auto it = mod->memberIndices.find(name.name); it != mod->memberIndices.end()) {
				result = mod->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Class: {
			AstNodePtr<ClassNode> m = memberNode.castTo<ClassNode>();

			if (auto it = m->memberIndices.find(name.name); it != m->memberIndices.end()) {
				result = m->members.at(it.value());
			} else {
				{
					AstNodePtr<ClassNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->baseType, baseType, nullptr));
					if (baseType) {
						SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, baseType.castTo<MemberNode>(), name, result));

						if (result) {
							goto classResolutionSucceeded;
						}
					}
				}

				for (auto &i : m->implTypes) {
					{
						AstNodePtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, baseType.castTo<MemberNode>(), name, result));

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
			AstNodePtr<InterfaceNode> m = memberNode.castTo<InterfaceNode>();

			if (auto it = m->memberIndices.find(name.name); it != m->memberIndices.end()) {
				result = m->members.at(it.value());
			} else {
				for (auto &i : m->implTypes) {
					{
						AstNodePtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, baseType.castTo<MemberNode>(), name, result));

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
		case AstNodeType::GenericParam: {
			AstNodePtr<GenericParamNode> m = memberNode.castTo<GenericParamNode>();

			{
				AstNodePtr<ClassNode> baseType;
				SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->genericConstraint->baseType, baseType, nullptr));
				if (baseType) {
					SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, baseType.castTo<MemberNode>(), name, result));

					if (result) {
						goto genericParamResolutionSucceeded;
					}
				}
			}

			for (auto &i : m->genericConstraint->implTypes) {
				{
					AstNodePtr<InterfaceNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, nullptr));
					if (baseType) {
						SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, baseType.castTo<MemberNode>(), name, result));

						if (result) {
							goto genericParamResolutionSucceeded;
						}
					}
				}
			}

		genericParamResolutionSucceeded:
			break;
		}
		case AstNodeType::This: {
			AstNodePtr<ThisNode> cls = memberNode.castTo<ThisNode>();

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, cls->document->sharedFromThis(), cls->thisType, name, result));

			break;
		}
		case AstNodeType::Var: {
			AstNodePtr<VarNode> m = memberNode.castTo<VarNode>();

			if (m->type->typeNameKind != TypeNameKind::Custom) {
				result = {};
				break;
			}

			AstNodePtr<TypeNameNode> type;
			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(m->type, type));

			AstNodePtr<MemberNode> tm;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, type.castTo<CustomTypeNameNode>(), tm));

			if (!tm) {
				result = {};
				break;
			}

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, tm, name, result));

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
				AstNodePtr<VarNode> m = result.castTo<VarNode>();

				// Check if the variable member is static or not.
				if (m->accessModifier & slake::ACCESS_STATIC) {
					return {};
				}
				break;
			}
			case AstNodeType::FnSlot: {
				AstNodePtr<FnNode> m = result.castTo<FnNode>();

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
	CompileEnvironment *compileEnv,
	peff::SharedPtr<Document> document,
	const AstNodePtr<MemberNode> &resolveRoot,
	IdRefEntry *idRef,
	size_t nEntries,
	AstNodePtr<MemberNode> &memberOut,
	ResolvedIdRefPartList *resolvedPartListOut,
	bool isStatic) {
	AstNodePtr<MemberNode> curMember = resolveRoot;

	bool isPostStaticParts = !isStatic;

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
			SLKC_RETURN_IF_COMP_ERROR(resolveStaticMember(compileEnv, document, curMember, curEntry, curMember));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, document, curMember, curEntry, curMember));
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

					assert(part.nEntries);

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
	CompileEnvironment *compileEnv,
	peff::SharedPtr<Document> document,
	peff::Set<AstNodePtr<MemberNode>> &walkedNodes,
	const AstNodePtr<MemberNode> &resolveScope,
	IdRefEntry *idRef,
	size_t nEntries,
	AstNodePtr<MemberNode> &memberOut,
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
			AstNodePtr<MemberNode> curScope = resolveScope;

		reresolveWithNewScope:
			switch (curScope->astNodeType) {
				case AstNodeType::Class: {
					AstNodePtr<ClassNode> m = curScope.castTo<ClassNode>();

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
					AstNodePtr<InterfaceNode> m = curScope.castTo<InterfaceNode>();

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
					AstNodePtr<FnOverloadingNode> m = curScope.castTo<FnOverloadingNode>();

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

	SLKC_RETURN_IF_COMP_ERROR(resolveIdRef(compileEnv, document, resolveScope, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

	if (!memberOut) {
		switch (resolveScope->astNodeType) {
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> m = resolveScope.castTo<ClassNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				{
					AstNodePtr<ClassNode> baseType;
					SLKC_RETURN_IF_COMP_ERROR(visitBaseClass(m->baseType, baseType, &walkedNodes));
					if (baseType) {
						SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, baseType.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

						if (memberOut) {
							return {};
						}
					}
				}
				walkedNodes.clear();

				for (auto &i : m->implTypes) {
					if (!walkedNodes.insert(m.castTo<MemberNode>())) {
						return genOutOfMemoryCompError();
					}
					{
						AstNodePtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, &walkedNodes));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, baseType.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

							if (memberOut) {
								return {};
							}
						}
					}
					walkedNodes.clear();
				}

				if (m->parent && (!isSealed)) {
					AstNodePtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					switch (p->astNodeType) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Module:
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, p.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

							if (memberOut) {
								return {};
							}
							break;
					}
				}
				break;
			}
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> m = resolveScope.castTo<InterfaceNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				for (auto &i : m->implTypes) {
					if (!walkedNodes.insert(m.castTo<MemberNode>())) {
						return genOutOfMemoryCompError();
					}
					{
						AstNodePtr<InterfaceNode> baseType;
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(i, baseType, &walkedNodes));
						if (baseType) {
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, baseType.castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, true));

							if (memberOut) {
								return {};
							}
						}
					}
					walkedNodes.clear();
				}

				if (m->parent && (!isSealed)) {
					AstNodePtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					switch (p->astNodeType) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Module:
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, p, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

							if (memberOut) {
								return {};
							}
							break;
					}
				}
				break;
			}
			case AstNodeType::Module: {
				AstNodePtr<ModuleNode> m = resolveScope.castTo<ModuleNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				if (m->parent) {
					AstNodePtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					switch (p->astNodeType) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Module:
							SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, p, idRef, nEntries, memberOut, resolvedPartListOut, isStatic));

							if (memberOut) {
								return {};
							}
							break;
					}
				}
				break;
			}
			case AstNodeType::Fn: {
				AstNodePtr<FnOverloadingNode> m = resolveScope.castTo<FnOverloadingNode>();

				if (!walkedNodes.insert(m.castTo<MemberNode>())) {
					return genOutOfMemoryCompError();
				}

				if (!m->parent)
					std::terminate();

				AstNodePtr<FnNode> slot;
				{
					AstNodePtr<MemberNode> p = m->parent->sharedFromThis().castTo<MemberNode>();

					if (p->astNodeType != AstNodeType::FnSlot)
						std::terminate();
					slot = p.castTo<FnNode>();
				}

				if (slot->parent) {
					SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(compileEnv, document, walkedNodes, slot->parent->sharedFromThis().castTo<MemberNode>(), idRef, nEntries, memberOut, resolvedPartListOut, isStatic, false));
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
	const AstNodePtr<CustomTypeNameNode> &typeName,
	AstNodePtr<MemberNode> &memberNodeOut,
	peff::Set<AstNodePtr<MemberNode>> *walkedNodes) {
	AstNodePtr<MemberNode> member;

	if (typeName->cachedResolveResult.isValid()) {
		member = typeName->cachedResolveResult.lock();
		goto resolved;
	}

	if (walkedNodes) {
		SLKC_RETURN_IF_COMP_ERROR(resolveIdRefWithScopeNode(nullptr, document, *walkedNodes, typeName->contextNode.lock(), typeName->idRefPtr->entries.data(), typeName->idRefPtr->entries.size(), member, nullptr));
	} else {
		peff::Set<AstNodePtr<MemberNode>> myWalkedNodes(document->allocator.get());
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

[[nodiscard]] SLKC_API
	std::optional<CompilationError>
	slkc::resolveBaseOverridenCustomTypeName(
		peff::SharedPtr<Document> document,
		const AstNodePtr<CustomTypeNameNode>& typeName,
		AstNodePtr<TypeNameNode>& typeNameOut) {
	AstNodePtr<MemberNode> member;

	SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, typeName, member, nullptr));

	if (!member) {
		typeNameOut = {};
		return {};
	}

	switch (member->astNodeType) {
		case AstNodeType::GenericParam: {
			auto gp = member.castTo<GenericParamNode>();

			if (gp->genericConstraint) {
				auto &c = gp->genericConstraint;

				if (c->baseType) {
					auto bt = c->baseType;

					switch (bt->typeNameKind) {
						case TypeNameKind::I8:
						case TypeNameKind::I16:
						case TypeNameKind::I32:
						case TypeNameKind::I64:
						case TypeNameKind::ISize:
						case TypeNameKind::U8:
						case TypeNameKind::U16:
						case TypeNameKind::U32:
						case TypeNameKind::U64:
						case TypeNameKind::USize:
						case TypeNameKind::F32:
						case TypeNameKind::F64:
						case TypeNameKind::String:
						case TypeNameKind::Bool:
						case TypeNameKind::Any:
						case TypeNameKind::Unpacking:
						case TypeNameKind::Fn:
						case TypeNameKind::Array:
							typeNameOut = bt;
							break;
					}
				}
			}
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::visitBaseClass(AstNodePtr<TypeNameNode> cls, AstNodePtr<ClassNode> &classOut, peff::Set<AstNodePtr<MemberNode>> *walkedNodes) {
	if (cls && (cls->typeNameKind == TypeNameKind::Custom)) {
		AstNodePtr<MemberNode> baseType;

		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(cls->document->sharedFromThis(), cls.castTo<CustomTypeNameNode>(), baseType, walkedNodes));

		if (baseType && (baseType->astNodeType == AstNodeType::Class)) {
			AstNodePtr<ClassNode> b = baseType.castTo<ClassNode>();
			bool isCyclicInherited;

			SLKC_RETURN_IF_COMP_ERROR(b->isCyclicInherited(isCyclicInherited));

			if (((!walkedNodes) || (!walkedNodes->contains(baseType))) && (!isCyclicInherited)) {
				classOut = b;
			}
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::visitBaseInterface(AstNodePtr<TypeNameNode> cls, AstNodePtr<InterfaceNode> &classOut, peff::Set<AstNodePtr<MemberNode>> *walkedNodes) {
	if (cls && (cls->typeNameKind == TypeNameKind::Custom)) {
		AstNodePtr<MemberNode> baseType;

		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(cls->document->sharedFromThis(), cls.castTo<CustomTypeNameNode>(), baseType, walkedNodes));

		if (baseType && (baseType->astNodeType == AstNodeType::Interface)) {
			AstNodePtr<InterfaceNode> b = baseType.castTo<InterfaceNode>();
			bool isCyclicInherited;

			SLKC_RETURN_IF_COMP_ERROR(b->isCyclicInherited(isCyclicInherited));

			if (((!walkedNodes) || (!walkedNodes->contains(baseType))) && (!isCyclicInherited)) {
				classOut = b;
			}
		}
	}

	return {};
}
