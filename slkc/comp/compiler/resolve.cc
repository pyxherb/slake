#include "../compiler.h"

using namespace slkc;

std::optional<CompilationError> Compiler::resolveStaticMember(
	TopLevelCompileContext *compileContext,
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
			SLKC_RETURN_IF_COMP_ERROR(compileContext->instantiateGenericObject(result, name.genericArgs, result));
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

std::optional<CompilationError> Compiler::resolveInstanceMember(
	TopLevelCompileContext *compileContext,
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
			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(compileContext, m->type, type));

			CustomTypeNameResolveContext resolveContext(compileContext->allocator.get());

			peff::SharedPtr<MemberNode> tm;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, type.castTo<CustomTypeNameNode>(), tm));

			if (!tm) {
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, tm, name, result));

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

std::optional<CompilationError> Compiler::resolveIdRef(
	TopLevelCompileContext *compileContext,
	const peff::SharedPtr<MemberNode> &resolveRoot,
	IdRef *idRef,
	peff::SharedPtr<MemberNode> &memberOut,
	bool isStatic) {
	peff::SharedPtr<MemberNode> curMember = resolveRoot;

	for (size_t i = 0; i < idRef->entries.size(); ++i) {
		const IdRefEntry &curEntry = idRef->entries.at(i);
		if (!isStatic) {
			SLKC_RETURN_IF_COMP_ERROR(resolveStaticMember(compileContext, curMember, curEntry, curMember));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, curMember, curEntry, curMember));
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

std::optional<CompilationError> Compiler::resolveCustomTypeName(
	TopLevelCompileContext *compileContext,
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

	SLKC_RETURN_IF_COMP_ERROR(resolveIdRef(compileContext, typeName->contextNode.lock(), typeName->idRefPtr.get(), member));

	if (!member) {
		switch (member->astNodeType) {
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> m = member.castTo<ClassNode>();

				if (m->baseType && (m->baseType->typeNameKind == TypeNameKind::Custom)) {
					peff::SharedPtr<MemberNode> bt;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(
						compileContext,
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
						compileContext,
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
						compileContext,
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
