#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> Compiler::resolveStaticMember(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<MemberNode> &memberNode,
	const IdRefEntry &name,
	peff::SharedPtr<MemberNode> &memberOut) {
	peff::SharedPtr<MemberNode> result;

	switch (memberNode->astNodeType) {
		case AstNodeType::Module: {
			peff::SharedPtr<ModuleNode> mod = memberNode.castTo<ModuleNode>();

			if (auto it = mod->members.find(name.name); it != mod->members.end()) {
				result = it.value();
			}

			break;
		}
		case AstNodeType::Class: {
			peff::SharedPtr<ClassNode> cls = memberNode.castTo<ClassNode>();

			if (auto it = cls->members.find(name.name); it != cls->members.end()) {
				result = it.value();
			}

			break;
		}
		case AstNodeType::Interface: {
			peff::SharedPtr<InterfaceNode> cls = memberNode.castTo<InterfaceNode>();

			if (auto it = cls->members.find(name.name); it != cls->members.end()) {
				result = it.value();
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
					return {};
				}
				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnSlotNode> m = result.castTo<FnSlotNode>();

				// TODO: Check if the slot contains any static method.
				break;
			}
			default:;
		}
		memberOut = result;
		return {};
	}

	return {};
}

SLKC_API std::optional<CompilationError> Compiler::resolveInstanceMember(
	peff::SharedPtr<Document> document,
	peff::SharedPtr<MemberNode> memberNode,
	const IdRefEntry &name,
	peff::SharedPtr<MemberNode> &memberOut) {
	peff::SharedPtr<MemberNode> result;

	switch (memberNode->astNodeType) {
		case AstNodeType::Module: {
			peff::SharedPtr<ModuleNode> mod = memberNode.castTo<ModuleNode>();

			if (auto it = mod->members.find(name.name); it != mod->members.end()) {
				result = it.value();
			}

			break;
		}
		case AstNodeType::Class: {
			peff::SharedPtr<ClassNode> cls = memberNode.castTo<ClassNode>();

			if (auto it = cls->members.find(name.name); it != cls->members.end()) {
				result = it.value();
			}

			break;
		}
		case AstNodeType::Interface: {
			peff::SharedPtr<InterfaceNode> cls = memberNode.castTo<InterfaceNode>();

			if (auto it = cls->members.find(name.name); it != cls->members.end()) {
				result = it.value();
			}

			break;
		}
		case AstNodeType::Var: {
			peff::SharedPtr<VarNode> m = memberNode.castTo<VarNode>();

			if (m->type->typeNameKind != TypeNameKind::Custom) {
				return {};
			}

			peff::SharedPtr<TypeNameNode> type;
			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(m->type, type));

			CustomTypeNameResolveContext resolveContext(document->allocator.get());

			peff::SharedPtr<MemberNode> tm;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, type.castTo<CustomTypeNameNode>(), tm));

			if (!tm) {
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(document, tm, name, result));

			break;
		}
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
			default:;
		}
		memberOut = result;
		return {};
	}

	return {};
}

SLKC_API std::optional<CompilationError> Compiler::resolveIdRef(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<MemberNode> &resolveRoot,
	IdRefEntry *idRef,
	size_t nEntries,
	peff::SharedPtr<MemberNode> &memberOut,
	bool isStatic) {
	peff::SharedPtr<MemberNode> curMember = resolveRoot;

	for (size_t i = 0; i < nEntries; ++i) {
		const IdRefEntry &curEntry = idRef[i];
		if (!isStatic) {
			SLKC_RETURN_IF_COMP_ERROR(resolveStaticMember(document, curMember, curEntry, curMember));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(document, curMember, curEntry, curMember));
		}

		switch (curMember->astNodeType) {
			case AstNodeType::Var:
				isStatic = false;
				break;
		}
	}

	memberOut = curMember;
	return {};
}

SLKC_API std::optional<CompilationError> Compiler::resolveCustomTypeName(
	peff::SharedPtr<Document> document,
	CustomTypeNameResolveContext &resolveContext,
	const peff::SharedPtr<CustomTypeNameNode> &typeName,
	peff::SharedPtr<MemberNode> &memberNodeOut) {
	peff::SharedPtr<MemberNode> member;

	if (typeName->cachedResolveResult.isValid()) {
		memberNodeOut = typeName->cachedResolveResult.lock();
		return {};
	}

	if (!resolveContext.resolvedMemberNodes.insert(typeName->contextNode.lock())) {
		return genOutOfMemoryCompError();
	}

	SLKC_RETURN_IF_COMP_ERROR(resolveIdRef(document, typeName->contextNode.lock(), typeName->idRefPtr->entries.data(), typeName->idRefPtr->entries.size(), member));

	if (!member) {
		switch (member->astNodeType) {
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> m = member.castTo<ClassNode>();

				if (m->baseType && (m->baseType->typeNameKind == TypeNameKind::Custom)) {
					peff::SharedPtr<MemberNode> bt;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(
						document,
						resolveContext,
						m->baseType.castTo<CustomTypeNameNode>(),
						bt));

					if (bt) {
						member = bt;
						goto succeeded;
					}
				}

				for (auto &i : m->implementedTypes) {
					if (i->typeNameKind != TypeNameKind::Custom) {
						continue;
					}
					peff::SharedPtr<MemberNode> it;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(
						document,
						resolveContext,
						i.castTo<CustomTypeNameNode>(),
						it));

					if (it) {
						member = it;
						goto succeeded;
					}
				}

				break;
			}
			case AstNodeType::Interface: {
				peff::SharedPtr<InterfaceNode> m = member.castTo<InterfaceNode>();

				for (auto &i : m->implementedTypes) {
					if (i->typeNameKind != TypeNameKind::Custom) {
						continue;
					}
					peff::SharedPtr<MemberNode> it;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(
						document,
						resolveContext,
						i.castTo<CustomTypeNameNode>(),
						it));

					if (it) {
						member = it;
						goto succeeded;
					}
				}
				break;
			}
			default:
				break;
		}
	}

succeeded:
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
