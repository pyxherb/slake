#include "../compiler.h"

using namespace slkc;

std::optional<CompilationError> Compiler::collectInvolvedInterfaces(
	FnCompileContext& compileContext,
	const peff::SharedPtr<InterfaceNode>& derived,
	peff::Set<peff::SharedPtr<InterfaceNode>> &walkedInterfaces,
	bool insertSelf) {
	if (walkedInterfaces.contains(derived)) {
		return {};
	}
	if (insertSelf) {
		if (!walkedInterfaces.insert(peff::SharedPtr<InterfaceNode>(derived))) {
			return genOutOfMemoryCompError();
		}
	}

	for (size_t i = 0; i < derived->implementedTypes.size(); ++i) {
		peff::SharedPtr<TypeNameNode> t = derived->implementedTypes.at(i);

		peff::SharedPtr<MemberNode> m;
		CustomTypeNameResolveContext resolveContext(compileContext.allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, t.castTo<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->astNodeType != AstNodeType::Interface) {
			goto malformed;
		}

		SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(compileContext, m.castTo<InterfaceNode>(), walkedInterfaces, true));
	}

	return {};

malformed:
	return {};
}

std::optional<CompilationError> Compiler::isImplementedByInterface(
	FnCompileContext& compileContext,
	const peff::SharedPtr<InterfaceNode>& base,
	const peff::SharedPtr<InterfaceNode>& derived,
	bool& whetherOut) {
	peff::Set<peff::SharedPtr<InterfaceNode>> interfaces(compileContext.allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(compileContext, derived, interfaces, true));

	whetherOut = interfaces.contains(base);
	return {};
}

std::optional<CompilationError> Compiler::isImplementedByClass(
	FnCompileContext& compileContext,
	const peff::SharedPtr<InterfaceNode>& base,
	const peff::SharedPtr<ClassNode>& derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<ClassNode>> walkedClasses(compileContext.allocator.get());

	if (!walkedClasses.insert(peff::SharedPtr<ClassNode>(derived))) {
		return genOutOfMemoryCompError();
	}

	peff::SharedPtr<ClassNode> currentClass = derived;
	peff::SharedPtr<TypeNameNode> currentType = derived->baseType;

	while (currentType) {
		if (currentType->typeNameKind != TypeNameKind::Custom) {
			goto malformed;
		}

		peff::SharedPtr<MemberNode> m;
		CustomTypeNameResolveContext resolveContext(compileContext.allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, currentType.castTo<CustomTypeNameNode>(), m));

		if (m->astNodeType != AstNodeType::Class) {
			goto malformed;
		}

		currentClass = m.castTo<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walkedClasses.contains(currentClass)) {
			goto malformed;
		}

		for (size_t i = 0; i < currentClass->implementedTypes.size(); ++i) {
			peff::SharedPtr<TypeNameNode> t = derived->implementedTypes.at(i);

			peff::SharedPtr<MemberNode> m;
			CustomTypeNameResolveContext resolveContext(compileContext.allocator.get());
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, t.castTo<CustomTypeNameNode>(), m));

			if (!m) {
				goto malformed;
			}

			if (m->astNodeType != AstNodeType::Interface) {
				goto malformed;
			}

			peff::SharedPtr<InterfaceNode> interfaceNode = m.castTo<InterfaceNode>();

			if (interfaceNode == base) {
				whetherOut = true;
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(compileContext, base, interfaceNode, whetherOut));

			if (whetherOut) {
				whetherOut = true;
				return {};
			}
		}

		if (!walkedClasses.insert(peff::SharedPtr<ClassNode>(currentClass))) {
			return genOutOfMemoryCompError();
		}
	}

	whetherOut = false;
	return {};

malformed:
	whetherOut = false;
	return {};
}

std::optional<CompilationError> Compiler::isBaseOf(
	FnCompileContext &compileContext,
	const peff::SharedPtr<ClassNode> &base,
	const peff::SharedPtr<ClassNode> &derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<ClassNode>> walkedClasses(compileContext.allocator.get());

	if (!walkedClasses.insert(peff::SharedPtr<ClassNode>(base))) {
		return genOutOfMemoryCompError();
	}
	if (!walkedClasses.insert(peff::SharedPtr<ClassNode>(derived))) {
		return genOutOfMemoryCompError();
	}

	peff::SharedPtr<ClassNode> currentClass = derived;
	peff::SharedPtr<TypeNameNode> currentType = derived->baseType;

	while (currentType) {
		if (currentType->typeNameKind != TypeNameKind::Custom) {
			goto malformed;
		}

		peff::SharedPtr<MemberNode> m;
		CustomTypeNameResolveContext resolveContext(compileContext.allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, currentType.castTo<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->astNodeType != AstNodeType::Class) {
			goto malformed;
		}

		currentClass = m.castTo<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walkedClasses.contains(currentClass)) {
			goto malformed;
		}

		if (currentClass == base) {
			whetherOut = true;
			return {};
		}

		if (!walkedClasses.insert(peff::SharedPtr<ClassNode>(currentClass))) {
			return genOutOfMemoryCompError();
		}
	}

	whetherOut = false;
	return {};

malformed:
	whetherOut = false;
	return {};
}

std::optional<CompilationError> Compiler::removeRefOfTypeName(
	FnCompileContext &compileContext,
	peff::SharedPtr<TypeNameNode> src,
	peff::SharedPtr<TypeNameNode> &typeNameOut) {
	switch (src->typeNameKind) {
		case TypeNameKind::Ref:
			typeNameOut = src.castTo<RefTypeNameNode>()->referencedType;
			break;
		default:
			typeNameOut = src;
	}

	return {};
}

std::optional<CompilationError> Compiler::isTypeNamesSame(
	FnCompileContext &compileContext,
	const peff::SharedPtr<TypeNameNode> &lhs,
	const peff::SharedPtr<TypeNameNode> &rhs,
	bool &whetherOut) {
	if (lhs->typeNameKind != rhs->typeNameKind) {
		whetherOut = false;
		return {};
	}

	switch (lhs->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<CustomTypeNameNode>
				convertedLhs = lhs.castTo<CustomTypeNameNode>(),
				convertedRhs = rhs.castTo<CustomTypeNameNode>();

			CustomTypeNameResolveContext resolveContext(compileContext.allocator.get());

			peff::SharedPtr<MemberNode> lhsMember, rhsMember;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, convertedLhs, lhsMember));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext, resolveContext, convertedRhs, rhsMember));

			whetherOut = lhsMember == rhsMember;
			break;
		}
		case TypeNameKind::Array: {
			peff::SharedPtr<ArrayTypeNameNode>
				convertedLhs = lhs.castTo<ArrayTypeNameNode>(),
				convertedRhs = rhs.castTo<ArrayTypeNameNode>();

			return isTypeNamesSame(compileContext, convertedLhs->elementType, convertedRhs->elementType, whetherOut);
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode>
				convertedLhs = lhs.castTo<RefTypeNameNode>(),
				convertedRhs = rhs.castTo<RefTypeNameNode>();

			return isTypeNamesSame(compileContext, convertedLhs->referencedType, convertedRhs->referencedType, whetherOut);
		}
		default:
			whetherOut = true;
	}

	return {};
}

std::optional<CompilationError> Compiler::isTypeNamesConvertible(
	FnCompileContext &compileContext,
	const peff::SharedPtr<TypeNameNode> &src,
	const peff::SharedPtr<TypeNameNode> &dest,
	bool &whetherOut) {
	switch (dest->typeNameKind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::F32:
		case TypeNameKind::F64:
		case TypeNameKind::ISize:
		case TypeNameKind::USize: {
			switch (src->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::U64:
				case TypeNameKind::F32:
				case TypeNameKind::F64:
				case TypeNameKind::ISize:
				case TypeNameKind::USize:
					whetherOut = true;
					break;
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::String: {
			switch (src->typeNameKind) {
				case TypeNameKind::String:
					whetherOut = true;
					break;
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::Bool: {
			switch (src->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::U64:
				case TypeNameKind::F32:
				case TypeNameKind::F64:
				case TypeNameKind::ISize:
				case TypeNameKind::USize:
				case TypeNameKind::String:
				case TypeNameKind::Object:
				case TypeNameKind::Bool:
					whetherOut = true;
					break;
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::Object: {
			switch (src->typeNameKind) {
				case TypeNameKind::String:
				case TypeNameKind::Custom:
					whetherOut = true;
					break;
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::Any:
			whetherOut = true;
			break;
		case TypeNameKind::Custom: {
			switch (src->typeNameKind) {
				case TypeNameKind::Custom: {
					peff::SharedPtr<CustomTypeNameNode>
						st = src.castTo<CustomTypeNameNode>(),
						dt = dest.castTo<CustomTypeNameNode>();
					whetherOut = true;
					break;
				}
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::Array:
			SLKC_RETURN_IF_COMP_ERROR(isTypeNamesSame(compileContext, src, dest, whetherOut));
			break;
		case TypeNameKind::Ref:
			SLKC_RETURN_IF_COMP_ERROR(isTypeNamesSame(compileContext, src, dest, whetherOut));
			whetherOut = false;
			break;
	}

	return {};
}
