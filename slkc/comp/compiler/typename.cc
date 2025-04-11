#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> Compiler::collectInvolvedInterfaces(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<InterfaceNode> &derived,
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
		CustomTypeNameResolveContext resolveContext(document->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, t.castTo<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->astNodeType != AstNodeType::Interface) {
			goto malformed;
		}

		SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(document, m.castTo<InterfaceNode>(), walkedInterfaces, true));
	}

	return {};

malformed:
	return {};
}

SLKC_API std::optional<CompilationError> Compiler::isImplementedByInterface(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<InterfaceNode> &base,
	const peff::SharedPtr<InterfaceNode> &derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<InterfaceNode>> interfaces(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(document, derived, interfaces, true));

	whetherOut = interfaces.contains(base);
	return {};
}

SLKC_API std::optional<CompilationError> Compiler::isImplementedByClass(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<InterfaceNode> &base,
	const peff::SharedPtr<ClassNode> &derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<ClassNode>> walkedClasses(document->allocator.get());

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
		CustomTypeNameResolveContext resolveContext(document->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, currentType.castTo<CustomTypeNameNode>(), m));

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
			CustomTypeNameResolveContext resolveContext(document->allocator.get());
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, t.castTo<CustomTypeNameNode>(), m));

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

			SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(document, base, interfaceNode, whetherOut));

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

SLKC_API std::optional<CompilationError> Compiler::isBaseOf(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<ClassNode> &base,
	const peff::SharedPtr<ClassNode> &derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<ClassNode>> walkedClasses(document->allocator.get());

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
		CustomTypeNameResolveContext resolveContext(document->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, currentType.castTo<CustomTypeNameNode>(), m));

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

SLKC_API std::optional<CompilationError> Compiler::removeRefOfType(
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

SLKC_API std::optional<CompilationError> Compiler::isSameType(
	const peff::SharedPtr<TypeNameNode> &lhs,
	const peff::SharedPtr<TypeNameNode> &rhs,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = lhs->document.lock();
	if (document != rhs->document.lock())
		std::terminate();

	if (lhs->typeNameKind != rhs->typeNameKind) {
		whetherOut = false;
		return {};
	}

	switch (lhs->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<CustomTypeNameNode>
				convertedLhs = lhs.castTo<CustomTypeNameNode>(),
				convertedRhs = rhs.castTo<CustomTypeNameNode>();

			CustomTypeNameResolveContext resolveContext(document->allocator.get());

			peff::SharedPtr<MemberNode> lhsMember, rhsMember;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, convertedLhs, lhsMember));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, resolveContext, convertedRhs, rhsMember));

			whetherOut = lhsMember == rhsMember;
			break;
		}
		case TypeNameKind::Array: {
			peff::SharedPtr<ArrayTypeNameNode>
				convertedLhs = lhs.castTo<ArrayTypeNameNode>(),
				convertedRhs = rhs.castTo<ArrayTypeNameNode>();

			return isSameType(convertedLhs->elementType, convertedRhs->elementType, whetherOut);
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode>
				convertedLhs = lhs.castTo<RefTypeNameNode>(),
				convertedRhs = rhs.castTo<RefTypeNameNode>();

			return isSameType(convertedLhs->referencedType, convertedRhs->referencedType, whetherOut);
		}
		default:
			whetherOut = true;
	}

	return {};
}

SLKC_API std::optional<CompilationError> Compiler::isTypeConvertible(
	const peff::SharedPtr<TypeNameNode> &src,
	const peff::SharedPtr<TypeNameNode> &dest,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = src->document.lock();
	if (document != dest->document.lock())
		std::terminate();

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
			SLKC_RETURN_IF_COMP_ERROR(isSameType(src, dest, whetherOut));
			break;
		case TypeNameKind::Ref:
			SLKC_RETURN_IF_COMP_ERROR(isSameType(src, dest, whetherOut));
			whetherOut = false;
			break;
	}

	return {};
}

SLKC_API std::optional<slkc::CompilationError> slkc::typeNameCmp(peff::SharedPtr<TypeNameNode> lhs, peff::SharedPtr<TypeNameNode> rhs, int &out) noexcept {
	peff::SharedPtr<Document> doc = lhs->document.lock();

	if (doc != rhs->document.lock())
		std::terminate();

	if (((uint8_t)lhs->typeNameKind) < ((uint8_t)rhs->typeNameKind)) {
		out = -1;
		return {};
	}
	if (((uint8_t)lhs->typeNameKind) > ((uint8_t)rhs->typeNameKind)) {
		out = 1;
		return {};
	}
	switch (lhs->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<CustomTypeNameNode>
				l = lhs.castTo<CustomTypeNameNode>(),
				r = rhs.castTo<CustomTypeNameNode>();

			peff::SharedPtr<MemberNode>
				lm,
				rm;

			{
				CustomTypeNameResolveContext resolveContext(doc->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(Compiler::resolveCustomTypeName(doc, resolveContext, l, lm));
			}
			{
				CustomTypeNameResolveContext resolveContext(doc->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(Compiler::resolveCustomTypeName(doc, resolveContext, r, rm));
			}

			if (!lm) {
				if (rm) {
					// [Bad type] > [Regular custom type]
					out = 1;
					return {};
				}
				// [Bad type] == [Bad type]
				out = 0;
				return {};
			}
			if (!rm) {
				out = -1;
				return {};
			}
			if (lm < rm) {
				out = -1;
			} else if (lm > rm) {
				out = 1;
			} else {
				out = 0;
			}
			return {};
		}
		case TypeNameKind::Array: {
			return typeNameCmp(
				lhs.castTo<ArrayTypeNameNode>()->elementType,
				rhs.castTo<ArrayTypeNameNode>()->elementType,
				out);
		}
		case TypeNameKind::Ref: {
			return typeNameCmp(
				lhs.castTo<RefTypeNameNode>()->referencedType,
				rhs.castTo<RefTypeNameNode>()->referencedType,
				out);
		}
		default:
			out = 0;
			return {};
	}

	std::terminate();
}

SLKC_API std::optional<slkc::CompilationError> slkc::typeNameListCmp(const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs, int &out) noexcept {
	if (lhs.size() < rhs.size()) {
		out = -1;
		return {};
	}
	if (lhs.size() > rhs.size()) {
		out = 1;
		return {};
	}
	for (size_t i = 0; i < lhs.size(); ++i) {
		SLKC_RETURN_IF_COMP_ERROR(typeNameCmp(lhs.at(i), rhs.at(i), out));

		if (out != 0) {
			return {};
		}
	}

	out = 0;
	return {};
}
