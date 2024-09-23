#include "../compiler.h"

using namespace slake::slkc;

bool Compiler::isLiteralTypeName(std::shared_ptr<TypeNameNode> typeName) {
	// stub
	return false;

	switch (typeName->getTypeId()) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::String:
		case TypeId::Bool:
			return true;
		case TypeId::Array: {
			return isLiteralTypeName(std::static_pointer_cast<ArrayTypeNameNode>(typeName));
		}
		case TypeId::Auto:
		case TypeId::Void:
		case TypeId::Any:
		case TypeId::Fn:
		case TypeId::Custom:
			return false;
		default:
			throw std::logic_error("Unrecognized type");
	}
}

bool Compiler::isNumericTypeName(std::shared_ptr<TypeNameNode> node) {
	switch (node->getTypeId()) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
			return true;
		default:
			return false;
	}
}

bool Compiler::isDecimalType(std::shared_ptr<TypeNameNode> node) {
	switch (node->getTypeId()) {
		case TypeId::F32:
		case TypeId::F64:
			return true;
		default:
			return false;
	}
}

bool Compiler::isCompoundTypeName(std::shared_ptr<TypeNameNode> node) {
	switch (node->getTypeId()) {
		case TypeId::Array:
		case TypeId::Fn:
			return true;
		case TypeId::Custom: {
			auto t = std::static_pointer_cast<CustomTypeNameNode>(node);
			auto dest = resolveCustomTypeName(t.get());
			switch (dest->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface:
				case NodeType::GenericParam:
					return true;
				case NodeType::Alias:
					return isCompoundTypeName(
						std::make_shared<CustomTypeNameNode>(
							t->ref,
							this,
							nullptr,
							t->isRef));
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(t->ref[0].tokenRange),
							MessageType::Error,
							"`" + std::to_string(t->ref, this) + "' cannot be referenced as a type"));
			}
		}
		default:
			return false;
	}
}

bool slake::slkc::Compiler::isLValueType(std::shared_ptr<TypeNameNode> typeName) {
	return typeName->isRef;
}

bool Compiler::_isTypeNamesConvertible(std::shared_ptr<ClassNode> st, std::shared_ptr<InterfaceNode> dt) {
	for (auto i : st->implInterfaces) {
		auto interface = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)i.get()));
		assert(interface->getNodeType() == NodeType::Interface);

		if (interface == dt)
			return true;

		for (auto j : st->implInterfaces) {
			auto jt = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)j.get()));
			assert(jt->getNodeType() == NodeType::Interface);

			if (_isTypeNamesConvertible(jt, dt))
				return true;
		}
	}

	return false;
}

bool Compiler::_isTypeNamesConvertible(std::shared_ptr<InterfaceNode> st, std::shared_ptr<ClassNode> dt) {
	for (auto i : dt->implInterfaces) {
		auto interface = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)i.get()));
		assert(interface->getNodeType() == NodeType::Interface);

		if (interface == st)
			return true;

		for (auto j : interface->parentInterfaces) {
			auto jt = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)j.get()));
			assert(jt->getNodeType() == NodeType::Interface);

			if (_isTypeNamesConvertible(jt, dt))
				return true;
		}
	}

	return false;
}

bool Compiler::_isTypeNamesConvertible(std::shared_ptr<InterfaceNode> st, std::shared_ptr<InterfaceNode> dt) {
	for (auto i : dt->parentInterfaces) {
		auto interface = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)i.get()));
		assert(interface->getNodeType() == NodeType::Interface);

		if (interface == st)
			return true;

		for (auto j : interface->parentInterfaces) {
			auto jt = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)j.get()));
			assert(jt->getNodeType() == NodeType::Interface);

			if (_isTypeNamesConvertible(jt, dt))
				return true;
		}
	}

	return false;
}

bool Compiler::_isTypeNamesConvertible(std::shared_ptr<ClassNode> st, std::shared_ptr<ClassNode> dt) {
	do {
		if (st == dt)
			return true;

		auto scope = scopeOf(st.get());
		assert(scope);
	} while (st->parentClass && (st = std::static_pointer_cast<ClassNode>(resolveCustomTypeName((CustomTypeNameNode *)st->parentClass.get()))));

	return false;
}

bool Compiler::isTypeNamesConvertible(std::shared_ptr<TypeNameNode> src, std::shared_ptr<TypeNameNode> dest) {
	if (dest->isRef) {
		if (!src->isRef)
			return false;
		return isSameType(src, dest);
	}

	if (((src->getTypeId()) == dest->getTypeId()) && (!isCompoundTypeName(src)))
		return true;

	/*if (src->getTypeId() == TypeId::Ref)
		return isTypeNamesConvertible(std::static_pointer_cast<RefTypeNameNode>(src)->referencedType, dest);*/

	switch (dest->getTypeId()) {
		case TypeId::Any:
			return true;
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
			if (isNumericTypeName(src))
				return true;
			if (src->getTypeId() == TypeId::Any)
				return true;
			break;
		case TypeId::Bool:
			return true;
		case TypeId::String:
		case TypeId::Array:
			if (src->getTypeId() == TypeId::Any)
				return true;
			break;
		case TypeId::Custom: {
			if (src->getTypeId() == TypeId::Any)
				return true;

			auto destType = resolveCustomTypeName((CustomTypeNameNode *)dest.get());

			switch (destType->getNodeType()) {
				case NodeType::Class: {
					auto dt = std::static_pointer_cast<ClassNode>(destType);

					if (src->getTypeId() == TypeId::Custom) {
						auto srcType = resolveCustomTypeName((CustomTypeNameNode *)src.get());

						switch (srcType->getNodeType()) {
							case NodeType::Class:
								return _isTypeNamesConvertible(std::static_pointer_cast<ClassNode>(srcType), dt);
							case NodeType::Interface:
								return _isTypeNamesConvertible(std::static_pointer_cast<InterfaceNode>(srcType), dt);
							default:
								throw std::logic_error("Unresolved node type");
						}
					}

					return false;
				}
				case NodeType::Interface: {
					auto dt = std::static_pointer_cast<InterfaceNode>(destType);

					if (src->getTypeId() == TypeId::Custom) {
						auto srcType = resolveCustomTypeName((CustomTypeNameNode *)src.get());

						switch (srcType->getNodeType()) {
							case NodeType::Class:
								return _isTypeNamesConvertible(std::static_pointer_cast<ClassNode>(srcType), dt);
							case NodeType::Interface:
								return _isTypeNamesConvertible(std::static_pointer_cast<InterfaceNode>(srcType), dt);
							case NodeType::GenericParam: {
								auto gp = std::static_pointer_cast<GenericParamNode>(srcType);

								for (auto &i : gp->interfaceTypes) {
									auto st = std::static_pointer_cast<InterfaceNode>(resolveCustomTypeName((CustomTypeNameNode *)i.get()));

									if (_isTypeNamesConvertible(st, dt))
										return true;
								}

								break;
							}
							default:
								throw std::logic_error("Unresolved node type");
						}
					}

					return false;
				}
				case NodeType::Alias: {
					auto dt = std::static_pointer_cast<AliasNode>(destType);
					break;
				}
				case NodeType::GenericParam: {
					auto dt = std::static_pointer_cast<GenericParamNode>(destType);

					if (dt->baseType) {
						if (!isTypeNamesConvertible(src, dt->baseType))
							return false;
					}

					for (auto &i : dt->interfaceTypes) {
						if (!isTypeNamesConvertible(src, i))
							return false;
					}

					return true;
				}
			}
			break;
		}
		case TypeId::Fn:
			if (src->getTypeId() == TypeId::Any)
				return true;
			return false;
		case TypeId::Void:
			return false;
		case TypeId::Auto:
			throw std::logic_error("Invalid destination type");
	}

	if (src->getTypeId() == TypeId::Custom) {
		auto t = std::static_pointer_cast<CustomTypeNameNode>(src);
		auto scope = scopeOf(t.get());

		assert(scope);

		if (scope->members.count("operator@" + std::to_string(dest, this)))
			return true;
		return false;
	}

	return false;
}

std::shared_ptr<AstNode> Compiler::_resolveCustomTypeName(CustomTypeNameNode *typeName, const std::set<Scope *> &resolvingScopes) {
	if (!typeName->cachedResolvedResult.expired())
		return typeName->cachedResolvedResult.lock();

	// Check if the type refers to a generic parameter.
	if ((typeName->ref.size() == 1) && (typeName->ref[0].genericArgs.empty())) {
		auto genericParam = lookupGenericParam(typeName->scope->owner->shared_from_this(), typeName->ref[0].name);
		if (genericParam) {
#if SLKC_WITH_LANGUAGE_SERVER
			// Update corresponding semantic information.
			updateTokenInfo(typeName->ref[0].idxToken, [this, &genericParam](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.correspondingMember = genericParam;
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.semanticType = SemanticType::TypeParam;
			});
#endif

			typeName->cachedResolvedResult = genericParam;
			return genericParam;
		}
	}

	// Check the type with scope where the type name is created.
	if (typeName->scope) {
		std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> resolvedPartsOut;
		IdRefResolveContext resolveContext;
		resolveContext.resolvingScopes = resolvingScopes;

		if (_resolveIdRef(typeName->scope, typeName->ref, resolvedPartsOut, resolveContext)) {
			if (resolvedPartsOut.size() > 1)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(typeName->tokenRange),
						MessageType::Error,
						"Expecting a static identifier"));

			typeName->cachedResolvedResult = resolvedPartsOut.back().second;
			return resolvedPartsOut.back().second;
		}
	}

	// Check the type with the global scope.
	{
		std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> resolvedPartsOut;

		if (resolveIdRef(typeName->ref, resolvedPartsOut, true)) {
			if (resolvedPartsOut.size() > 1)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(typeName->tokenRange),
						MessageType::Error,
						"Expecting a static identifier"));

			typeName->cachedResolvedResult = resolvedPartsOut.back().second;
			return resolvedPartsOut.back().second;
		}
	}

	// Cannot resolve the type name - generate an error.
	throw FatalCompilationError(
		Message(
			tokenRangeToSourceLocation(typeName->tokenRange),
			MessageType::Error,
			"Type `" + std::to_string(typeName->ref, this) + "' was not found"));
}

std::shared_ptr<AstNode> Compiler::resolveCustomTypeName(CustomTypeNameNode *typeName) {
	std::set<Scope *> resolvingScopes;
	return _resolveCustomTypeName(typeName, resolvingScopes);
}

bool Compiler::isSameType(std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y) {
	/* if (x->getTypeId() == TypeId::Ref)
		x = std::static_pointer_cast<RefTypeNameNode>(x)->referencedType;
	if (y->getTypeId() == TypeId::Ref)
		y = std::static_pointer_cast<RefTypeNameNode>(y)->referencedType;*/

	if (x->getTypeId() != y->getTypeId())
		return false;

	switch (x->getTypeId()) {
		case TypeId::Custom: {
			std::shared_ptr<AstNode> xDest = resolveCustomTypeName((CustomTypeNameNode *)x.get()),
									 yDest = resolveCustomTypeName((CustomTypeNameNode *)y.get());

			auto xDestNodeType = xDest->getNodeType();

			if (xDestNodeType != yDest->getNodeType())
				return false;

			switch (xDestNodeType) {
				case NodeType::GenericParam: {
					// Only for isFnOverloadingDuplicated()
					std::shared_ptr<GenericParamNode> xGenericParam = std::static_pointer_cast<GenericParamNode>(xDest),
													  yGenericParam = std::static_pointer_cast<GenericParamNode>(yDest);

					AstNode *xNode = xGenericParam->ownerNode,
							*yNode = yGenericParam->ownerNode;

					if (xNode->getNodeType() != yNode->getNodeType())
						return false;

					switch (xNode->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface:
							return xGenericParam == yGenericParam;
						case NodeType::FnOverloadingValue:
							return xGenericParam->index == yGenericParam->index;
						default:
							throw std::logic_error("Unhandled owner type");
					}
					break;
				}
				default:
					return xDest == yDest;
			}
			break;
		}
		case TypeId::Array:
			return isSameType(
				std::static_pointer_cast<ArrayTypeNameNode>(x)->elementType,
				std::static_pointer_cast<ArrayTypeNameNode>(y)->elementType);
		default:
			return true;
	}
}

int Compiler::getTypeNameWeight(std::shared_ptr<TypeNameNode> t) {
	switch (t->getTypeId()) {
		case TypeId::Bool:
			return 0;
		case TypeId::I8:
			return 10;
		case TypeId::I16:
			return 11;
		case TypeId::I32:
			return 12;
		case TypeId::I64:
			return 13;
		case TypeId::U8:
			return 20;
		case TypeId::U16:
			return 21;
		case TypeId::U32:
			return 22;
		case TypeId::U64:
			return 23;
		case TypeId::F32:
			return 31;
		case TypeId::F64:
			return 32;
		default:
			return -1;
	}
}

std::shared_ptr<TypeNameNode> Compiler::getStrongerTypeName(std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y) {
	int leftWeight = getTypeNameWeight(x), rightWeight = getTypeNameWeight(y);

	if (rightWeight > leftWeight)
		return y;
	return x;
}

std::shared_ptr<AstNode> CustomTypeNameNode::doDuplicate() {
	return std::make_shared<CustomTypeNameNode>(*this);
}

std::shared_ptr<AstNode> ArrayTypeNameNode::doDuplicate() {
	return std::make_shared<ArrayTypeNameNode>(*this);
}

std::shared_ptr<AstNode> FnTypeNameNode::doDuplicate() {
	return std::make_shared<FnTypeNameNode>(*this);
}

std::shared_ptr<AstNode> slake::slkc::ContextTypeNameNode::doDuplicate() {
	return std::make_shared<ContextTypeNameNode>(*this);
}
