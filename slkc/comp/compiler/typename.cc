#include "../compiler.h"
#include <climits>

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::collectInvolvedInterfaces(
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

	for (size_t i = 0; i < derived->implTypes.size(); ++i) {
		peff::SharedPtr<TypeNameNode> t = derived->implTypes.at(i);

		peff::SharedPtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, t.castTo<CustomTypeNameNode>(), m));

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

SLKC_API std::optional<CompilationError> slkc::isImplementedByInterface(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<InterfaceNode> &base,
	const peff::SharedPtr<InterfaceNode> &derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<InterfaceNode>> interfaces(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(document, derived, interfaces, true));

	whetherOut = interfaces.contains(base);
	return {};
}

SLKC_API std::optional<CompilationError> slkc::isImplementedByClass(
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
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, currentType.castTo<CustomTypeNameNode>(), m));

		if (m->astNodeType != AstNodeType::Class) {
			goto malformed;
		}

		currentClass = m.castTo<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walkedClasses.contains(currentClass)) {
			whetherOut = true;
			return {};
		}

		for (size_t i = 0; i < currentClass->implTypes.size(); ++i) {
			peff::SharedPtr<TypeNameNode> t = derived->implTypes.at(i);

			peff::SharedPtr<MemberNode> m;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, t.castTo<CustomTypeNameNode>(), m));

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

SLKC_API std::optional<CompilationError> slkc::isBaseOf(
	peff::SharedPtr<Document> document,
	const peff::SharedPtr<ClassNode> &base,
	const peff::SharedPtr<ClassNode> &derived,
	bool &whetherOut) {
	peff::Set<peff::SharedPtr<ClassNode>> walkedClasses(document->allocator.get());

	if (!walkedClasses.insert(peff::SharedPtr<ClassNode>(derived))) {
		return genOutOfMemoryCompError();
	}

	peff::SharedPtr<ClassNode> currentClass = derived;
	peff::SharedPtr<TypeNameNode> currentType;

	while ((currentType = currentClass->baseType)) {
		if (currentType->typeNameKind != TypeNameKind::Custom) {
			goto malformed;
		}

		peff::SharedPtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, currentType.castTo<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->astNodeType != AstNodeType::Class) {
			goto malformed;
		}

		currentClass = m.castTo<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walkedClasses.contains(currentClass)) {
			whetherOut = true;
			return {};
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

SLKC_API std::optional<CompilationError> slkc::removeRefOfType(
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

SLKC_API std::optional<CompilationError> slkc::isLValueType(
	peff::SharedPtr<TypeNameNode> src,
	bool &whetherOut) {
	if (!src) {
		whetherOut = false;
		return {};
	}

	switch (src->typeNameKind) {
		case TypeNameKind::Ref:
			whetherOut = true;
			break;
		default:
			whetherOut = false;
			break;
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::isSameType(
	const peff::SharedPtr<TypeNameNode> &lhs,
	const peff::SharedPtr<TypeNameNode> &rhs,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = lhs->document->sharedFromThis();
	if (document != rhs->document->sharedFromThis())
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

			peff::SharedPtr<MemberNode> lhsMember, rhsMember;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, convertedLhs, lhsMember));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, convertedRhs, rhsMember));

			whetherOut = lhsMember == rhsMember;
			break;
		}
		case TypeNameKind::Array: {
			peff::SharedPtr<ArrayTypeNameNode>
				convertedLhs = lhs.castTo<ArrayTypeNameNode>(),
				convertedRhs = rhs.castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(isSameType(convertedLhs->elementType, convertedRhs->elementType, whetherOut));
			break;
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode>
				convertedLhs = lhs.castTo<RefTypeNameNode>(),
				convertedRhs = rhs.castTo<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(isSameType(convertedLhs->referencedType, convertedRhs->referencedType, whetherOut));
			break;
		}
		default:
			whetherOut = true;
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::getTypePromotionLevel(
	const peff::SharedPtr<TypeNameNode> &typeName,
	int &levelOut) {
	switch (typeName->typeNameKind) {
		case TypeNameKind::Bool:
			levelOut = 1;
			break;
		case TypeNameKind::I8:
			levelOut = 11;
			break;
		case TypeNameKind::I16:
			levelOut = 12;
			break;
		case TypeNameKind::I32:
			levelOut = 13;
			break;
		case TypeNameKind::I64:
			levelOut = 14;
			break;
		case TypeNameKind::U8:
			levelOut = 21;
			break;
		case TypeNameKind::U16:
			levelOut = 22;
			break;
		case TypeNameKind::U32:
			levelOut = 23;
			break;
		case TypeNameKind::U64:
			levelOut = 24;
			break;
		case TypeNameKind::F32:
			levelOut = 31;
			break;
		case TypeNameKind::F64:
			levelOut = 32;
			break;
		case TypeNameKind::Any:
			levelOut = INT_MAX;
			break;
		default:
			levelOut = INT_MIN + 1;
			break;
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::determinePromotionalType(
	peff::SharedPtr<TypeNameNode> lhs,
	peff::SharedPtr<TypeNameNode> rhs,
	peff::SharedPtr<TypeNameNode> &typeNameOut) {
	int lhsWeight, rhsWeight;

	if (!lhs) {
		typeNameOut = rhs;
		return {};
	}

	if (!rhs) {
		typeNameOut = lhs;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(getTypePromotionLevel(lhs, lhsWeight));
	SLKC_RETURN_IF_COMP_ERROR(getTypePromotionLevel(rhs, rhsWeight));

	if (lhsWeight < rhsWeight) {
		typeNameOut = lhs;
	} else if (lhsWeight > rhsWeight) {
		typeNameOut = rhs;
	} else {
		switch (lhs->typeNameKind) {
			case TypeNameKind::Array: {
				switch (rhs->typeNameKind) {
					case TypeNameKind::Array: {
						peff::SharedPtr<ArrayTypeNameNode> lt = lhs.castTo<ArrayTypeNameNode>(), rt = rhs.castTo<ArrayTypeNameNode>();
						peff::SharedPtr<TypeNameNode> finalType;

						SLKC_RETURN_IF_COMP_ERROR(determinePromotionalType(lt->elementType, rt->elementType, finalType));

						typeNameOut = finalType == rt->elementType ? rhs : lhs;
						break;
					}
					default:
						typeNameOut = lhs;
						break;
				}
				break;
			}
			case TypeNameKind::Custom: {
				switch (rhs->typeNameKind) {
					case TypeNameKind::Custom: {
						peff::SharedPtr<CustomTypeNameNode> lt = lhs.castTo<CustomTypeNameNode>(), rt = rhs.castTo<CustomTypeNameNode>();

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(rhs, lhs, true, b));
						if (b) {
							// In sealed context, derived types cannot be converted to base types,
							// hence when rhs can be converted to lhs, rhs is the base type.
							typeNameOut = rhs;
						} else {
							typeNameOut = lhs;
						}
						break;
					}
					default:
						typeNameOut = lhs;
						break;
				}
				break;
			}
			default:
				typeNameOut = lhs;
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::isSameTypeInSignature(
	const peff::SharedPtr<TypeNameNode> &lhs,
	const peff::SharedPtr<TypeNameNode> &rhs,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = lhs->document->sharedFromThis();
	if (document != rhs->document->sharedFromThis())
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

			peff::SharedPtr<MemberNode> lhsMember, rhsMember;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, convertedLhs, lhsMember));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, convertedRhs, rhsMember));

			if ((!lhsMember) || (!rhsMember)) {
				if ((!lhsMember) != (!rhsMember)) {
					whetherOut = false;
					break;
				} else {
					whetherOut = true;
					break;
				}
			}

			if (lhsMember->astNodeType != rhsMember->astNodeType) {
				whetherOut = false;
				break;
			}

			switch (lhsMember->astNodeType) {
				case AstNodeType::GenericParam: {
					peff::SharedPtr<GenericParamNode> l, r;

					l = lhsMember.castTo<GenericParamNode>();
					r = rhsMember.castTo<GenericParamNode>();

					if (((FnOverloadingNode *)l->parent)->genericParamIndices.at(l->name) == ((FnOverloadingNode *)r->parent)->genericParamIndices.at(r->name)) {
						whetherOut = true;
						break;
					}

					whetherOut = false;
					break;
				}
				default:
					whetherOut = lhsMember == rhsMember;
					break;
			}
			break;
		}
		default:
			SLKC_RETURN_IF_COMP_ERROR(isSameType(lhs, rhs, whetherOut));
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::isTypeConvertible(
	const peff::SharedPtr<TypeNameNode> &src,
	const peff::SharedPtr<TypeNameNode> &dest,
	bool isSealed,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = src->document->sharedFromThis();
	if (document != dest->document->sharedFromThis())
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
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src.castTo<RefTypeNameNode>()->referencedType, dest, isSealed, whetherOut));
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
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src.castTo<RefTypeNameNode>()->referencedType, dest, isSealed, whetherOut));
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
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src.castTo<RefTypeNameNode>()->referencedType, dest, isSealed, whetherOut));
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
					if (isSealed) {
						return isSameType(src, dest, whetherOut);
					}
					whetherOut = true;
					break;
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src.castTo<RefTypeNameNode>()->referencedType, dest, isSealed, whetherOut));
					break;
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::Any:
			if (isSealed) {
				return isSameType(src, dest, whetherOut);
			}
			whetherOut = true;
			break;
		case TypeNameKind::Custom: {
			switch (src->typeNameKind) {
				case TypeNameKind::Custom: {
					peff::SharedPtr<CustomTypeNameNode>
						st = src.castTo<CustomTypeNameNode>(),
						dt = dest.castTo<CustomTypeNameNode>();

					peff::SharedPtr<MemberNode> stm, dtm;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, st, stm));
					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, dt, dtm));

					switch (stm->astNodeType) {
						case AstNodeType::Class:
							switch (dtm->astNodeType) {
								case AstNodeType::Interface:
									SLKC_RETURN_IF_COMP_ERROR(isImplementedByClass(document, dtm.castTo<InterfaceNode>(), stm.castTo<ClassNode>(), whetherOut));
									break;
								case AstNodeType::Class:
									SLKC_RETURN_IF_COMP_ERROR(isBaseOf(document, dtm.castTo<ClassNode>(), stm.castTo<ClassNode>(), whetherOut));
									if ((!isSealed) && (!whetherOut)) {
										// Covariance is not allowed in sealed context.
										SLKC_RETURN_IF_COMP_ERROR(isBaseOf(document, stm.castTo<ClassNode>(), dtm.castTo<ClassNode>(), whetherOut));
									}
									break;
								case AstNodeType::GenericParam: {
									auto dgp = dtm.castTo<GenericParamNode>();

									if (dgp->genericConstraint->baseType && dgp->genericConstraint->baseType->typeNameKind == TypeNameKind::Custom) {
										SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src, dgp->genericConstraint->baseType, isSealed, whetherOut));

										if (whetherOut) {
											break;
										}
									}
									break;
								}
							}
							break;
						case AstNodeType::Interface:
							switch (dtm->astNodeType) {
								case AstNodeType::Interface:
									SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(document, dtm.castTo<InterfaceNode>(), stm.castTo<InterfaceNode>(), whetherOut));
									if ((!isSealed) && (!whetherOut)) {
										// Covariance is not allowed in sealed context.
										SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(document, stm.castTo<InterfaceNode>(), dtm.castTo<InterfaceNode>(), whetherOut));
									}
									break;
								case AstNodeType::Class:
									SLKC_RETURN_IF_COMP_ERROR(isImplementedByClass(document, stm.castTo<InterfaceNode>(), dtm.castTo<ClassNode>(), whetherOut));
									break;
								case AstNodeType::GenericParam: {
									auto dgp = dtm.castTo<GenericParamNode>();

									if (dgp->genericConstraint->baseType && dgp->genericConstraint->baseType->typeNameKind == TypeNameKind::Custom) {
										SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src, dgp->genericConstraint->baseType, isSealed, whetherOut));

										if (whetherOut) {
											break;
										}
									}

									for (auto i : dgp->genericConstraint->implTypes) {
										SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src, i, isSealed, whetherOut));

										if (whetherOut) {
											break;
										}
									}
									break;
								}
							}
							break;
						case AstNodeType::GenericParam: {
							auto sgp = stm.castTo<GenericParamNode>();

							switch (dtm->astNodeType) {
								case AstNodeType::Interface:
									if (sgp->genericConstraint->baseType && sgp->genericConstraint->baseType->typeNameKind == TypeNameKind::Custom) {
										SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(sgp->genericConstraint->baseType, dest, isSealed, whetherOut));
									}
									for (auto i : sgp->genericConstraint->implTypes) {
										SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(i, dest, isSealed, whetherOut));

										if (whetherOut) {
											break;
										}
									}
									break;
								case AstNodeType::Class:
									if (sgp->genericConstraint->baseType && sgp->genericConstraint->baseType->typeNameKind == TypeNameKind::Custom) {
										SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(sgp->genericConstraint->baseType, dest, isSealed, whetherOut));
									}
									break;
								case AstNodeType::GenericParam:
									// Direct conversions between generic parameters are disabled.
									break;
							}
							break;
						}
						default:
							whetherOut = false;
							break;
					}

					break;
				}
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(src.castTo<RefTypeNameNode>()->referencedType, dest, isSealed, whetherOut));
					break;
				default:
					whetherOut = false;
			}
			break;
		}
		case TypeNameKind::Array:
			switch (src->typeNameKind) {
				case TypeNameKind::Ref: {
					SLKC_RETURN_IF_COMP_ERROR(isSameType(src.castTo<RefTypeNameNode>()->referencedType, dest, whetherOut));
					break;
				}
				default:
					SLKC_RETURN_IF_COMP_ERROR(isSameType(src, dest, whetherOut));
					break;
			}
			break;
		case TypeNameKind::Ref:
			switch (src->typeNameKind) {
				case TypeNameKind::Ref: {
					SLKC_RETURN_IF_COMP_ERROR(isSameType(src, dest, whetherOut));
					break;
				}
				default:
					// RValue to LValue is not allowed.
					whetherOut = false;
					break;
			}
			break;
		case TypeNameKind::TempRef:
			SLKC_RETURN_IF_COMP_ERROR(isSameType(src, dest, whetherOut));
			whetherOut = false;
			break;
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::_isTypeNameParamListTypeNameTree(
	peff::SharedPtr<TypeNameNode> type,
	bool &whetherOut) {
	if (!type) {
		whetherOut = false;
		return {};
	}

	switch (type->typeNameKind) {
		case TypeNameKind::Unpacking: {
			whetherOut = true;
			break;
		}
		case TypeNameKind::Array: {
			auto t = type.castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(t->elementType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		case TypeNameKind::Ref: {
			auto t = type.castTo<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(t->referencedType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		case TypeNameKind::TempRef: {
			auto t = type.castTo<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(t->referencedType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		case TypeNameKind::Fn: {
			auto t = type.castTo<FnTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(t->returnType, whetherOut));
			if (whetherOut)
				return {};

			for (auto &i : t->paramTypes) {
				SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(i, whetherOut));
				if (whetherOut)
					return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(t->thisType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		default:
			break;
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::_doExpandParamListTypeNameTree(
	peff::SharedPtr<TypeNameNode> &type) {
	if (!type) {
		return {};
	}

	switch (type->typeNameKind) {
		case TypeNameKind::Unpacking: {
			SLKC_RETURN_IF_COMP_ERROR(getUnpackedTypeOf(type, type));
			break;
		}
		case TypeNameKind::Array: {
			auto t = type.castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(t->elementType));

			break;
		}
		case TypeNameKind::Ref: {
			auto t = type.castTo<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(t->referencedType));

			break;
		}
		case TypeNameKind::TempRef: {
			auto t = type.castTo<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(t->referencedType));

			break;
		}
		case TypeNameKind::Fn: {
			auto t = type.castTo<FnTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(t->returnType));

			for (auto &i : t->paramTypes) {
				SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(i));
			}

			SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(t->thisType));

			break;
		}
		default:
			break;
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::simplifyParamListTypeNameTree(
	peff::SharedPtr<TypeNameNode> type,
	peff::Alloc *allocator,
	peff::SharedPtr<TypeNameNode> &typeNameOut) {
	bool b;

	SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(type, b));

	if (!b) {
		typeNameOut = type;
		return {};
	}

	peff::SharedPtr<TypeNameNode> duplicatedType = type->duplicate<TypeNameNode>(allocator);

	if (!duplicatedType) {
		return genOutOfMemoryCompError();
	}

	SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(duplicatedType));

	typeNameOut = duplicatedType;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::_isTypeNameGenericParamFacade(
	peff::SharedPtr<TypeNameNode> type,
	bool& whetherOut) {
	if (!type) {
		whetherOut = false;
		return {};
	}

	switch (type->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<TypeNameNode> t;

			SLKC_RETURN_IF_COMP_ERROR(resolveBaseOverridenCustomTypeName(type->document->sharedFromThis(), type.castTo<CustomTypeNameNode>(), t));

			if (t) {
				whetherOut = true;
				return {};
			}
			break;
		}
		case TypeNameKind::Array: {
			auto t = type.castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(t->elementType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		case TypeNameKind::Ref: {
			auto t = type.castTo<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(t->referencedType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		case TypeNameKind::TempRef: {
			auto t = type.castTo<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(t->referencedType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		case TypeNameKind::Fn: {
			auto t = type.castTo<FnTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(t->returnType, whetherOut));
			if (whetherOut)
				return {};

			for (auto &i : t->paramTypes) {
				SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(i, whetherOut));
				if (whetherOut)
					return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(t->thisType, whetherOut));
			if (whetherOut)
				return {};

			break;
		}
		default:
			break;
	}

	whetherOut = false;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::_doExpandGenericParamFacadeTypeNameTree(
	peff::SharedPtr<TypeNameNode> &type) {
	if (!type) {
		return {};
	}

	switch (type->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<TypeNameNode> t;

			SLKC_RETURN_IF_COMP_ERROR(resolveBaseOverridenCustomTypeName(type->document->sharedFromThis(), type.castTo<CustomTypeNameNode>(), t));

			if (t)
				type = t;
			break;
		}
		case TypeNameKind::Array: {
			auto t = type.castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(t->elementType));

			break;
		}
		case TypeNameKind::Ref: {
			auto t = type.castTo<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(t->referencedType));

			break;
		}
		case TypeNameKind::TempRef: {
			auto t = type.castTo<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(t->referencedType));

			break;
		}
		case TypeNameKind::Fn: {
			auto t = type.castTo<FnTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(t->returnType));

			for (auto &i : t->paramTypes) {
				SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(i));
			}

			SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(t->thisType));

			break;
		}
		default:
			break;
	}

	return {};
}

[[nodiscard]] SLKC_API std::optional<CompilationError> slkc::simplifyGenericParamFacadeTypeNameTree(
	peff::SharedPtr<TypeNameNode> type,
	peff::Alloc* allocator,
	peff::SharedPtr<TypeNameNode> &typeNameOut) {
	bool b;

	SLKC_RETURN_IF_COMP_ERROR(_isTypeNameGenericParamFacade(type, b));

	if (!b) {
		typeNameOut = type;
		return {};
	}

	peff::SharedPtr<TypeNameNode> duplicatedType = type->duplicate<TypeNameNode>(allocator);

	if (!duplicatedType) {
		return genOutOfMemoryCompError();
	}

	SLKC_RETURN_IF_COMP_ERROR(_doExpandGenericParamFacadeTypeNameTree(duplicatedType));

	typeNameOut = duplicatedType;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::getUnpackedTypeOf(
	peff::SharedPtr<TypeNameNode> type,
	peff::SharedPtr<TypeNameNode> &typeNameOut) {
	peff::SharedPtr<Document> document = type->document->sharedFromThis();

	switch (type->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, type.castTo<CustomTypeNameNode>(), m));

			if (!m) {
				return CompilationError(type->tokenRange, CompilationErrorKind::IdNotFound);
			}

			switch (m->astNodeType) {
				case AstNodeType::GenericParam: {
					auto p = m.castTo<GenericParamNode>();

					if (p->isParamTypeList) {
						peff::SharedPtr<UnpackedParamsTypeNameNode> unpackedType;

						if (!(unpackedType = peff::makeSharedWithControlBlock<UnpackedParamsTypeNameNode, AstNodeControlBlock<UnpackedParamsTypeNameNode>>(document->allocator.get(), document->allocator.get(), document))) {
							return genOutOfMemoryCompError();
						}

						if (p->paramTypeListGenericConstraint) {
							if (!unpackedType->paramTypes.resize(p->paramTypeListGenericConstraint->argTypes.size())) {
								return genOutOfMemoryCompError();
							}

							for (size_t i = 0; i < unpackedType->paramTypes.size(); ++i) {
								unpackedType->paramTypes.at(i) = p->paramTypeListGenericConstraint->argTypes.at(i);
							}
						}

						if (p->paramTypeListGenericConstraint) {
							unpackedType->hasVarArgs = p->paramTypeListGenericConstraint->hasVarArg;
						}

						typeNameOut = unpackedType.castTo<TypeNameNode>();
					} else {
						typeNameOut = {};
					}
					break;
				}
				default:
					typeNameOut = {};
			}
			break;
		}
		case TypeNameKind::ParamTypeList: {
			auto t = type.castTo<ParamTypeListTypeNameNode>();

			peff::SharedPtr<UnpackedParamsTypeNameNode> unpackedType;

			if (!(unpackedType = peff::makeSharedWithControlBlock<UnpackedParamsTypeNameNode, AstNodeControlBlock<UnpackedParamsTypeNameNode>>(document->allocator.get(), document->allocator.get(), document))) {
				return genOutOfMemoryCompError();
			}

			if (!unpackedType->paramTypes.resize(t->paramTypes.size())) {
				return genOutOfMemoryCompError();
			}

			for (size_t i = 0; i < unpackedType->paramTypes.size(); ++i) {
				unpackedType->paramTypes.at(i) = t->paramTypes.at(i);
			}

			unpackedType->hasVarArgs = t->hasVarArgs;

			typeNameOut = unpackedType.castTo<TypeNameNode>();
			break;
		}
		case TypeNameKind::UnpackedParams: {
			auto t = type.castTo<UnpackedParamsTypeNameNode>();

			peff::SharedPtr<UnpackedArgsTypeNameNode> unpackedType;

			if (!(unpackedType = peff::makeSharedWithControlBlock<UnpackedArgsTypeNameNode, AstNodeControlBlock<UnpackedArgsTypeNameNode>>(document->allocator.get(), document->allocator.get(), document))) {
				return genOutOfMemoryCompError();
			}

			if (!unpackedType->paramTypes.resize(t->paramTypes.size())) {
				return genOutOfMemoryCompError();
			}

			for (size_t i = 0; i < unpackedType->paramTypes.size(); ++i) {
				unpackedType->paramTypes.at(i) = t->paramTypes.at(i);
			}

			unpackedType->hasVarArgs = t->hasVarArgs;

			typeNameOut = unpackedType.castTo<TypeNameNode>();

			break;
		}
		case TypeNameKind::Unpacking: {
			SLKC_RETURN_IF_COMP_ERROR(getUnpackedTypeOf(type.castTo<UnpackingTypeNameNode>()->innerTypeName, typeNameOut));

			break;
		}
		default:
			typeNameOut = {};
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::fnToTypeName(
	CompileEnvironment *compileEnv,
	peff::SharedPtr<FnOverloadingNode> fn,
	peff::SharedPtr<FnTypeNameNode> &evaluatedTypeOut) {
	peff::SharedPtr<FnTypeNameNode> tn;

	if (!(tn = peff::makeSharedWithControlBlock<FnTypeNameNode, AstNodeControlBlock<FnTypeNameNode>>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	if (!tn->paramTypes.resize(fn->params.size())) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
		tn->paramTypes.at(i) = fn->params.at(i)->type;
	}

	if (fn->fnFlags & FN_VARG) {
		tn->hasVarArgs = true;
	}

	tn->returnType = fn->returnType;

	if (!(fn->accessModifier & slake::ACCESS_STATIC)) {
		if (fn->parent && fn->parent->parent) {
			switch (fn->parent->parent->astNodeType) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					IdRefPtr fullIdRef;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), fn->parent->parent->sharedFromThis().castTo<MemberNode>(), fullIdRef));

					auto thisType = peff::makeSharedWithControlBlock<CustomTypeNameNode, AstNodeControlBlock<CustomTypeNameNode>>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document);

					if (!thisType) {
						return genOutOfMemoryCompError();
					}
					thisType->contextNode = compileEnv->document->rootModule.castTo<MemberNode>();

					thisType->idRefPtr = std::move(fullIdRef);

					tn->thisType = thisType.castTo<TypeNameNode>();
					break;
				}
				default:
					break;
			}
		}
	}

	evaluatedTypeOut = tn;

	return {};
}

SLKC_API std::optional<slkc::CompilationError> slkc::typeNameCmp(peff::SharedPtr<TypeNameNode> lhs, peff::SharedPtr<TypeNameNode> rhs, int &out) noexcept {
	peff::SharedPtr<Document> doc = lhs->document->sharedFromThis();

	if (doc != rhs->document->sharedFromThis())
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

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(doc, l, lm));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(doc, r, rm));

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
