#include "../compiler.h"
#include <climits>

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::removeRefOfType(
	AstNodePtr<TypeNameNode> src,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(src, src));

	switch (src->typeNameKind) {
		case TypeNameKind::Ref:
			typeNameOut = src.castTo<RefTypeNameNode>()->referencedType;
			break;
		default:
			typeNameOut = src;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isLValueType(
	AstNodePtr<TypeNameNode> src,
	bool &whetherOut) {
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(src, src));

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

SLKC_API peff::Option<CompilationError> slkc::isSameType(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = lhs->document->sharedFromThis();
	if (document != rhs->document->sharedFromThis())
		std::terminate();

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(lhs, lhs));
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(rhs, rhs));

	if (lhs->typeNameKind != rhs->typeNameKind) {
		whetherOut = false;
		return {};
	}

	if (lhs->isFinal != rhs->isFinal) {
		whetherOut = false;
		return {};
	}

	switch (lhs->typeNameKind) {
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode>
				convertedLhs = lhs.castTo<CustomTypeNameNode>(),
				convertedRhs = rhs.castTo<CustomTypeNameNode>();

			AstNodePtr<MemberNode> lhsMember, rhsMember;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, convertedLhs, lhsMember));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, convertedRhs, rhsMember));

			whetherOut = lhsMember == rhsMember;
			break;
		}
		case TypeNameKind::Array: {
			AstNodePtr<ArrayTypeNameNode>
				convertedLhs = lhs.castTo<ArrayTypeNameNode>(),
				convertedRhs = rhs.castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(isSameType(convertedLhs->elementType, convertedRhs->elementType, whetherOut));
			break;
		}
		case TypeNameKind::Ref: {
			AstNodePtr<RefTypeNameNode>
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

SLKC_API peff::Option<CompilationError> slkc::getTypePromotionLevel(
	AstNodePtr<TypeNameNode> typeName,
	int &levelOut) {
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(typeName, typeName));

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
			levelOut = INT_MAX - 1;
			break;
		default:
			levelOut = INT_MAX;
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::determinePromotionalType(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	int lhsWeight, rhsWeight;

	if (!lhs) {
		typeNameOut = rhs;
		return {};
	}

	if (!rhs) {
		typeNameOut = lhs;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(lhs, lhs));
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(rhs, rhs));

	SLKC_RETURN_IF_COMP_ERROR(getTypePromotionLevel(lhs, lhsWeight));
	SLKC_RETURN_IF_COMP_ERROR(getTypePromotionLevel(rhs, rhsWeight));

	if (lhsWeight < rhsWeight) {
		typeNameOut = rhs;
	} else if (lhsWeight > rhsWeight) {
		typeNameOut = lhs;
	} else {
		switch (lhs->typeNameKind) {
			case TypeNameKind::Array: {
				switch (rhs->typeNameKind) {
					case TypeNameKind::Array: {
						AstNodePtr<ArrayTypeNameNode> lt = lhs.castTo<ArrayTypeNameNode>(), rt = rhs.castTo<ArrayTypeNameNode>();
						AstNodePtr<TypeNameNode> finalType;

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
						AstNodePtr<CustomTypeNameNode> lt = lhs.castTo<CustomTypeNameNode>(), rt = rhs.castTo<CustomTypeNameNode>();

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(rhs, lhs, true, b));
						if (b) {
							// In sealed context, derived types cannot be converted to base types,
							// therefore when rhs can be converted to lhs, rhs is the base type.
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

SLKC_API peff::Option<CompilationError> slkc::isSubtypeOf(
	AstNodePtr<TypeNameNode> subtype,
	AstNodePtr<TypeNameNode> type,
	bool &resultOut) {
recheck:
	switch (subtype->typeNameKind) {
		case TypeNameKind::Void:
			resultOut = false;
			break;
		case TypeNameKind::I8:
			switch (type->typeNameKind) {
				case TypeNameKind::I8:
					resultOut = false;
					break;
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::ISize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::I16:
			switch (type->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
					resultOut = false;
					break;
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::ISize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::I32:
			switch (type->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
					resultOut = false;
					break;
				case TypeNameKind::ISize:
				case TypeNameKind::I64:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::I64:
			switch (type->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
					resultOut = false;
					break;
				case TypeNameKind::ISize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::ISize:
			switch (type->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::ISize:
					resultOut = false;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::U8:
			switch (type->typeNameKind) {
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::U64:
				case TypeNameKind::USize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::U16:
			switch (type->typeNameKind) {
				case TypeNameKind::U8:
				case TypeNameKind::U16:
					resultOut = false;
					break;
				case TypeNameKind::U32:
				case TypeNameKind::U64:
				case TypeNameKind::USize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::U32:
			switch (type->typeNameKind) {
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
					resultOut = false;
					break;
				case TypeNameKind::U64:
				case TypeNameKind::USize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::U64:
			switch (type->typeNameKind) {
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::U64:
					resultOut = false;
					break;
				case TypeNameKind::USize:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::USize:
			switch (type->typeNameKind) {
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::USize:
				case TypeNameKind::U64:
					resultOut = false;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::F32:
			switch (type->typeNameKind) {
				case TypeNameKind::F32:
					resultOut = false;
					break;
				case TypeNameKind::F64:
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::F64:
			switch (type->typeNameKind) {
				case TypeNameKind::F32:
				case TypeNameKind::F64:
					resultOut = false;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::String:
			switch (type->typeNameKind) {
				case TypeNameKind::Object:
					resultOut = true;
					break;
				case TypeNameKind::String:
					resultOut = false;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::Bool:
			switch (type->typeNameKind) {
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
					resultOut = true;
					break;
				default:
					resultOut = false;
					break;
			}
			break;
		case TypeNameKind::Object:
		case TypeNameKind::Any:
			resultOut = false;
			break;
		case TypeNameKind::Ref: {
			switch (type->typeNameKind) {
				case TypeNameKind::Ref:
					SLKC_RETURN_IF_COMP_ERROR(isSameType(subtype.castTo<RefTypeNameNode>()->referencedType, type.castTo<RefTypeNameNode>()->referencedType, resultOut));
					break;
				default:
					SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(subtype, subtype));
					goto recheck;
			}
			break;
		}
		case TypeNameKind::Custom:
			switch (type->typeNameKind) {
				case TypeNameKind::Object: {
					AstNodePtr<MemberNode> stm;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, type->document->sharedFromThis(), type.castTo<CustomTypeNameNode>(), stm));

					if (stm->getAstNodeType() == AstNodeType::Class)
						resultOut = true;
					else
						resultOut = false;
					break;
				}
				case TypeNameKind::Custom: {
					AstNodePtr<MemberNode> stm, tm;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, type->document->sharedFromThis(), type.castTo<CustomTypeNameNode>(), tm));
					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, subtype->document->sharedFromThis(), subtype.castTo<CustomTypeNameNode>(), stm));

					switch (tm->getAstNodeType()) {
						case AstNodeType::Class:
							switch (stm->getAstNodeType()) {
								case AstNodeType::Class:
									SLKC_RETURN_IF_COMP_ERROR(isBaseOf(type->document->sharedFromThis(), tm.castTo<ClassNode>(), stm.castTo<ClassNode>(), resultOut));
									break;
								default:
									resultOut = false;
									break;
							}
							break;
						case AstNodeType::Interface:
							switch (stm->getAstNodeType()) {
								case AstNodeType::Class:
									SLKC_RETURN_IF_COMP_ERROR(isImplementedByClass(type->document->sharedFromThis(), tm.castTo<InterfaceNode>(), stm.castTo<ClassNode>(), resultOut));
									break;
								case AstNodeType::Interface:
									SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(type->document->sharedFromThis(), tm.castTo<InterfaceNode>(), stm.castTo<InterfaceNode>(), resultOut));
									break;
								case AstNodeType::Struct:
									// TODO: Process struct here...
									resultOut = false;
									break;
							}
							break;
						default:
							resultOut = false;
							break;
					}
					break;
				}
				default:
					resultOut = false;
					break;
			}
			break;
		default:
			resultOut = false;
			break;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isUnsigned(
	AstNodePtr<TypeNameNode> type,
	bool &resultOut) {
	switch (type->typeNameKind) {
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::USize:
			resultOut = true;
			break;
		default:
			resultOut = false;
	}
	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::toSigned(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	switch (type->typeNameKind) {
		case TypeNameKind::U8:
			if (!(typeNameOut = makeAstNode<I8TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::U16:
			if (!(typeNameOut = makeAstNode<I16TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::U32:
			if (!(typeNameOut = makeAstNode<I32TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::U64:
			if (!(typeNameOut = makeAstNode<I64TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::USize:
			if (!(typeNameOut = makeAstNode<ISizeTypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		default:
			typeNameOut = {};
			break;
	}

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::toUnsigned(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	switch (type->typeNameKind) {
		case TypeNameKind::I8:
			if (!(typeNameOut = makeAstNode<U8TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::I16:
			if (!(typeNameOut = makeAstNode<U16TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::I32:
			if (!(typeNameOut = makeAstNode<U32TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::I64:
			if (!(typeNameOut = makeAstNode<U64TypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		case TypeNameKind::ISize:
			if (!(typeNameOut = makeAstNode<USizeTypeNameNode>(
					  type->selfAllocator.get(),
					  type->selfAllocator.get(),
					  type->document->sharedFromThis())
						.castTo<TypeNameNode>()))
				return genOutOfMemoryCompError();
			break;
		default:
			typeNameOut = {};
			break;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isFloatingPoint(
	AstNodePtr<TypeNameNode> type,
	bool &resultOut) {
	switch (type->typeNameKind) {
		case TypeNameKind::F32:
		case TypeNameKind::F64:
			resultOut = true;
			break;
		default:
			resultOut = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isSigned(
	AstNodePtr<TypeNameNode> type,
	bool &resultOut) {
	switch (type->typeNameKind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::ISize:
			resultOut = true;
			break;
		default:
			resultOut = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isIntegral(
	AstNodePtr<TypeNameNode> type,
	bool &resultOut) {
	switch (type->typeNameKind) {
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
			resultOut = true;
			break;
		default:
			resultOut = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isBasicType(
	AstNodePtr<TypeNameNode> type,
	bool &resultOut) {
	switch (type->typeNameKind) {
		case TypeNameKind::Void:
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
		case TypeNameKind::Object:
		case TypeNameKind::Any:
			resultOut = true;
			break;
		default:
			resultOut = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isScopedEnumBaseType(
	AstNodePtr<TypeNameNode> lhs,
	bool &resultOut) {
	switch (lhs->typeNameKind) {
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
		case TypeNameKind::Bool:
			resultOut = true;
			break;
		default:
			resultOut = false;
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::deduceCommonType(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	AstNodePtr<TypeNameNode> &typeNameOut) {
reconvert: {
	bool isSame;
	SLKC_RETURN_IF_COMP_ERROR(isSameType(lhs, rhs, isSame));
	if (isSame) {
		typeNameOut = lhs;
		return {};
	}
}

	bool whether;
	SLKC_RETURN_IF_COMP_ERROR(isSubtypeOf(lhs, rhs, whether));
	if (whether) {
		typeNameOut = rhs;
		return {};
	}
	SLKC_RETURN_IF_COMP_ERROR(isSubtypeOf(rhs, lhs, whether));
	if (whether) {
		typeNameOut = lhs;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(isUnsigned(lhs, whether));
	if (whether) {
		SLKC_RETURN_IF_COMP_ERROR(isSigned(rhs, whether));
		if (whether) {
			// l = unsigned , r = signed
			SLKC_RETURN_IF_COMP_ERROR(toUnsigned(rhs, rhs));
			goto reconvert;
		}
	} else {
		SLKC_RETURN_IF_COMP_ERROR(isUnsigned(rhs, whether));
		if (whether) {
			AstNodePtr<TypeNameNode> tmp;
			SLKC_RETURN_IF_COMP_ERROR(toUnsigned(lhs, lhs));
			if (tmp) {
				rhs = tmp;
				goto reconvert;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(isFloatingPoint(lhs, whether));
	if (whether) {
		SLKC_RETURN_IF_COMP_ERROR(isFloatingPoint(rhs, whether));
		if (!whether) {
			// l = FP, r = non-FP
			switch (rhs->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
					if (!(rhs = makeAstNode<F32TypeNameNode>(
							  rhs->selfAllocator.get(),
							  rhs->selfAllocator.get(),
							  rhs->document->sharedFromThis())
								.castTo<TypeNameNode>()))
						return genOutOfMemoryCompError();
					goto reconvert;
				case TypeNameKind::I64:
				case TypeNameKind::U64:
					if (!(rhs = makeAstNode<F64TypeNameNode>(
							  rhs->selfAllocator.get(),
							  rhs->selfAllocator.get(),
							  rhs->document->sharedFromThis())
								.castTo<TypeNameNode>()))
						return genOutOfMemoryCompError();
					goto reconvert;
			}
		}
		// l = FP, r = ??? where r is not unsigned nor signed.
	} else {
		SLKC_RETURN_IF_COMP_ERROR(isFloatingPoint(rhs, whether));
		if (whether) {
			// l = FP, r = non-FP
			switch (lhs->typeNameKind) {
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
					if (!(lhs = makeAstNode<F32TypeNameNode>(
							  lhs->selfAllocator.get(),
							  lhs->selfAllocator.get(),
							  lhs->document->sharedFromThis())
								.castTo<TypeNameNode>()))
						return genOutOfMemoryCompError();
					goto reconvert;
				case TypeNameKind::I64:
				case TypeNameKind::U64:
					if (!(lhs = makeAstNode<F64TypeNameNode>(
							  lhs->selfAllocator.get(),
							  lhs->selfAllocator.get(),
							  lhs->document->sharedFromThis())
								.castTo<TypeNameNode>()))
						return genOutOfMemoryCompError();
					goto reconvert;
			}
		}
		// l = ???, r = FP where r is not unsigned nor signed.
	}

	// Failed - I give up.
	typeNameOut = {};
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isSameTypeInSignature(
	AstNodePtr<TypeNameNode> lhs,
	AstNodePtr<TypeNameNode> rhs,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = lhs->document->sharedFromThis();
	if (document != rhs->document->sharedFromThis())
		std::terminate();

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(lhs, lhs));
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(rhs, rhs));

	if (lhs->typeNameKind != rhs->typeNameKind) {
		whetherOut = false;
		return {};
	}

	switch (lhs->typeNameKind) {
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode>
				convertedLhs = lhs.castTo<CustomTypeNameNode>(),
				convertedRhs = rhs.castTo<CustomTypeNameNode>();

			AstNodePtr<MemberNode> lhsMember, rhsMember;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, convertedLhs, lhsMember));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, convertedRhs, rhsMember));

			if ((!lhsMember) || (!rhsMember)) {
				if ((!lhsMember) != (!rhsMember)) {
					whetherOut = false;
					break;
				} else {
					whetherOut = true;
					break;
				}
			}

			if (lhsMember->getAstNodeType() != rhsMember->getAstNodeType()) {
				whetherOut = false;
				break;
			}

			switch (lhsMember->getAstNodeType()) {
				case AstNodeType::GenericParam: {
					// TODO: Lookup the generic parameters recursively for classes, interfaces
					// and functions.
					AstNodePtr<GenericParamNode> l, r;

					l = lhsMember.castTo<GenericParamNode>();
					r = rhsMember.castTo<GenericParamNode>();

					auto lp = l->parent,
						 rp = r->parent;

					if (lp->getAstNodeType() != rp->getAstNodeType()) {
						whetherOut = false;
						break;
					}

					switch (lp->getAstNodeType()) {
						case AstNodeType::Class: {
							if (lp != rp) {
								whetherOut = false;
								break;
							}

							if (((ClassNode *)lp)->genericParamIndices.at(l->name) ==
								((ClassNode *)rp)->genericParamIndices.at(r->name)) {
								whetherOut = true;
								break;
							} else {
								whetherOut = false;
								break;
							}
							break;
						}
						case AstNodeType::Interface: {
							if (lp != rp) {
								whetherOut = false;
								break;
							}

							if (((InterfaceNode *)lp)->genericParamIndices.at(l->name) ==
								((InterfaceNode *)rp)->genericParamIndices.at(r->name)) {
								whetherOut = true;
								break;
							} else {
								whetherOut = false;
								break;
							}
							break;
						}
						case AstNodeType::FnOverloading: {
							auto lit = ((FnOverloadingNode *)lp)->genericParamIndices.find(l->name),
								 rit = ((FnOverloadingNode *)rp)->genericParamIndices.find(r->name);

							assert((lit != ((FnOverloadingNode *)lp)->genericParamIndices.end()) &&
								   (rit != ((FnOverloadingNode *)rp)->genericParamIndices.end()));

							if (*lit == *rit) {
								whetherOut = true;
								break;
							} else {
								whetherOut = false;
								break;
							}
							break;
						}
					}
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

SLKC_API peff::Option<CompilationError> slkc::isTypeConvertible(
	AstNodePtr<TypeNameNode> src,
	AstNodePtr<TypeNameNode> dest,
	bool isSealed,
	bool &whetherOut) {
	peff::SharedPtr<Document> document = src->document->sharedFromThis();
	if (document != dest->document->sharedFromThis())
		std::terminate();

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(src, src));
	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(dest, dest));

	if (dest->isFinal)
		isSealed = true;

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
					AstNodePtr<CustomTypeNameNode>
						st = src.castTo<CustomTypeNameNode>(),
						dt = dest.castTo<CustomTypeNameNode>();

					AstNodePtr<MemberNode> stm, dtm;

					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, st, stm));
					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, dt, dtm));

					switch (stm->getAstNodeType()) {
						case AstNodeType::Class:
							switch (dtm->getAstNodeType()) {
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
							switch (dtm->getAstNodeType()) {
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

							switch (dtm->getAstNodeType()) {
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

SLKC_API peff::Option<CompilationError> slkc::_isTypeNameParamListTypeNameTree(
	AstNodePtr<TypeNameNode> type,
	bool &whetherOut) {
	if (!type) {
		whetherOut = false;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(type, type));

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

SLKC_API peff::Option<CompilationError> slkc::_doExpandParamListTypeNameTree(
	AstNodePtr<TypeNameNode> &type) {
	if (!type) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(type, type));

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

SLKC_API peff::Option<CompilationError> slkc::unwrapParamListTypeNameTree(
	AstNodePtr<TypeNameNode> type,
	peff::Alloc *allocator,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	bool b;

	SLKC_RETURN_IF_COMP_ERROR(_isTypeNameParamListTypeNameTree(type, b));

	if (!b) {
		typeNameOut = type;
		return {};
	}

	AstNodePtr<TypeNameNode> duplicatedType = type->duplicate<TypeNameNode>(allocator);

	if (!duplicatedType) {
		return genOutOfMemoryCompError();
	}

	SLKC_RETURN_IF_COMP_ERROR(_doExpandParamListTypeNameTree(duplicatedType));

	typeNameOut = duplicatedType;

	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::unwrapFacadeTypeName(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	if (!type) {
		typeNameOut = type;
		return {};
	}
	switch (type->typeNameKind) {
		case TypeNameKind::Custom: {
			AstNodePtr<TypeNameNode> t;

			SLKC_RETURN_IF_COMP_ERROR(resolveBaseOverridenCustomTypeName(type->document->sharedFromThis(), type.castTo<CustomTypeNameNode>(), t));

			if (t) {
				typeNameOut = t;
				return {};
			}
			break;
		}
		default:
			break;
	}

	typeNameOut = type;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::getUnpackedTypeOf(
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<TypeNameNode> &typeNameOut) {
	peff::SharedPtr<Document> document = type->document->sharedFromThis();

	SLKC_RETURN_IF_COMP_ERROR(unwrapFacadeTypeName(type, type));
	switch (type->typeNameKind) {
		case TypeNameKind::Custom: {
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, document, type.castTo<CustomTypeNameNode>(), m));

			if (!m) {
				return CompilationError(type->tokenRange, CompilationErrorKind::IdNotFound);
			}

			switch (m->getAstNodeType()) {
				case AstNodeType::GenericParam: {
					auto p = m.castTo<GenericParamNode>();

					if (p->isParamTypeList) {
						AstNodePtr<UnpackedParamsTypeNameNode> unpackedType;

						if (!(unpackedType = makeAstNode<UnpackedParamsTypeNameNode>(document->allocator.get(), document->allocator.get(), document))) {
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

			AstNodePtr<UnpackedParamsTypeNameNode> unpackedType;

			if (!(unpackedType = makeAstNode<UnpackedParamsTypeNameNode>(document->allocator.get(), document->allocator.get(), document))) {
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

			AstNodePtr<UnpackedArgsTypeNameNode> unpackedType;

			if (!(unpackedType = makeAstNode<UnpackedArgsTypeNameNode>(document->allocator.get(), document->allocator.get(), document))) {
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

SLKC_API peff::Option<CompilationError> slkc::fnToTypeName(
	CompileEnvironment *compileEnv,
	AstNodePtr<FnOverloadingNode> fn,
	AstNodePtr<FnTypeNameNode> &evaluatedTypeOut) {
	AstNodePtr<FnTypeNameNode> tn;

	if (!(tn = makeAstNode<FnTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
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
			switch (fn->parent->parent->getAstNodeType()) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					IdRefPtr fullIdRef;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), fn->parent->parent->sharedFromThis().castTo<MemberNode>(), fullIdRef));

					auto thisType = makeAstNode<CustomTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document);

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

SLKC_API peff::Option<slkc::CompilationError> slkc::typeNameCmp(AstNodePtr<TypeNameNode> lhs, AstNodePtr<TypeNameNode> rhs, int &out) noexcept {
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
			AstNodePtr<CustomTypeNameNode>
				l = lhs.castTo<CustomTypeNameNode>(),
				r = rhs.castTo<CustomTypeNameNode>();

			AstNodePtr<MemberNode>
				lm,
				rm;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, doc, l, lm));
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(nullptr, doc, r, rm));

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

SLKC_API peff::Option<slkc::CompilationError> slkc::typeNameListCmp(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs, int &out) noexcept {
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
