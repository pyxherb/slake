#include "../compiler.h"

using namespace slake::slkc;

bool Compiler::isLiteralTypeName(std::shared_ptr<TypeNameNode> typeName) {
	// stub
	return false;

	switch (typeName->getTypeId()) {
		case Type::I8:
		case Type::I16:
		case Type::I32:
		case Type::I64:
		case Type::U8:
		case Type::U16:
		case Type::U32:
		case Type::U64:
		case Type::F32:
		case Type::F64:
		case Type::String:
		case Type::Bool:
			return true;
		case Type::Array: {
			return isLiteralTypeName(std::static_pointer_cast<ArrayTypeNameNode>(typeName));
		}
		case Type::Auto:
		case Type::Void:
		case Type::Any:
		case Type::Fn:
		case Type::Custom:
			return false;
		default:
			throw std::logic_error("Unrecognized type");
	}
}

bool Compiler::isNumericTypeName(std::shared_ptr<TypeNameNode> node) {
	switch (node->getTypeId()) {
		case Type::I8:
		case Type::I16:
		case Type::I32:
		case Type::I64:
		case Type::U8:
		case Type::U16:
		case Type::U32:
		case Type::U64:
		case Type::F32:
		case Type::F64:
			return true;
		default:
			return false;
	}
}

bool Compiler::isDecimalType(std::shared_ptr<TypeNameNode> node) {
	switch (node->getTypeId()) {
		case Type::F32:
		case Type::F64:
			return true;
		default:
			return false;
	}
}

bool Compiler::isCompoundTypeName(std::shared_ptr<TypeNameNode> node) {
	switch (node->getTypeId()) {
		case Type::Array:
		case Type::Fn:
			return true;
		case Type::Custom: {
			auto t = std::static_pointer_cast<CustomTypeNameNode>(node);
			auto dest = resolveCustomTypeName(t.get());
			switch (dest->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface:
				case NodeType::Trait:
					return true;
				case NodeType::Alias:
					return isCompoundTypeName(
						std::make_shared<CustomTypeNameNode>(
							Location(t->getLocation()),
							t->ref,
							this,
							nullptr,
							t->isConst));
				case NodeType::GenericArgRef:
					// stub
					return true;
				default:
					throw FatalCompilationError(
						Message(
							t->ref[0].loc,
							MessageType::Error,
							"`" + std::to_string(t->ref, this) + "' cannot be referenced as a type"));
			}
		}
		default:
			return false;
	}
}

bool slake::slkc::Compiler::isLValueType(std::shared_ptr<TypeNameNode> typeName) {
	switch (typeName->getTypeId()) {
		case Type::Ref:
			return true;
		case Type::Custom:
			// stub
		default:;
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

bool Compiler::_isTypeNamesConvertible(std::shared_ptr<MemberNode> st, std::shared_ptr<TraitNode> dt) {
	for (auto i : dt->scope->members) {
		if (!(scopeOf(st.get())->members.count(i.first)))
			return false;

		auto sm = scopeOf(st.get())->members.at(i.first);
		auto dm = i.second;

		if (sm->getNodeType() != dm->getNodeType())
			return false;

		// The member must be public
		if (!(sm->access & ACCESS_PUB))
			return false;

		// Mutability of the member must be stronger than the target type's.
		if ((sm->access & ACCESS_CONST) && !(dm->access & ACCESS_CONST))
			return false;

		switch (sm->getNodeType()) {
			case NodeType::Var: {
				auto smType = std::static_pointer_cast<VarNode>(sm);
				auto dmType = std::static_pointer_cast<VarNode>(dm);

				// The variables must have the same type.
				if (!isSameType(smType->type, dmType->type))
					return false;

				break;
			}
			case NodeType::Fn: {
				auto smType = std::static_pointer_cast<FnNode>(sm);
				auto dmType = std::static_pointer_cast<FnNode>(dm);

				for (auto di : dmType->overloadingRegistries) {
					for (auto si : smType->overloadingRegistries) {
						if (!isSameType(di->returnType, si->returnType))
							return false;

						if (di->params.size() != si->params.size())
							return false;

						for (size_t i = 0; i < di->params.size(); ++i) {
							if (!isSameType(si->params[i]->type, di->params[i]->type))
								return false;
						}
					}
				}

				break;
			}
			case NodeType::Class:
			case NodeType::Interface:
			case NodeType::Trait:
			case NodeType::Module:
			case NodeType::Alias:
				break;
			default:
				throw std::logic_error("Unrecognized member type");
		}
	}

	return false;
}

bool Compiler::isTypeNamesConvertible(std::shared_ptr<TypeNameNode> src, std::shared_ptr<TypeNameNode> dest) {
	if (((src->getTypeId()) == dest->getTypeId()) && (!isCompoundTypeName(src)))
		return true;

	switch (dest->getTypeId()) {
		case Type::Any:
			return true;
		case Type::I8:
		case Type::I16:
		case Type::I32:
		case Type::I64:
		case Type::U8:
		case Type::U16:
		case Type::U32:
		case Type::U64:
		case Type::F32:
		case Type::F64:
			if (isNumericTypeName(src))
				return true;
			break;
		case Type::Bool:
			return true;
		case Type::String:
		case Type::WString:
		case Type::Array:
			break;
		case Type::Custom: {
			auto destType = resolveCustomTypeName((CustomTypeNameNode *)dest.get());

			switch (destType->getNodeType()) {
				case NodeType::Class: {
					auto dt = std::static_pointer_cast<ClassNode>(destType);

					if (src->getTypeId() == Type::Custom) {
						auto srcType = resolveCustomTypeName((CustomTypeNameNode *)src.get());

						switch (srcType->getNodeType()) {
							case NodeType::Class: {
								auto st = std::static_pointer_cast<ClassNode>(srcType);

								do {
									if (st == dt)
										return true;

									auto scope = scopeOf(st.get());
									assert(scope);

									if (scope->members.count(std::string("operator") + (dest->getTypeId() == Type::Custom ? "" : "@") + std::to_string(dest, this)))
										return true;
								} while (st->parentClass && (st = std::static_pointer_cast<ClassNode>(resolveCustomTypeName((CustomTypeNameNode *)st->parentClass.get()))));

								break;
							}
							case NodeType::Interface:
								return _isTypeNamesConvertible(std::static_pointer_cast<InterfaceNode>(srcType), dt);
							case NodeType::Trait:
								return _isTypeNamesConvertible(dt, std::static_pointer_cast<TraitNode>(srcType));
							default:
								throw std::logic_error("Unresolved node type");
						}
					}

					return false;
				}
				case NodeType::Interface: {
					auto dt = std::static_pointer_cast<InterfaceNode>(destType);

					if (src->getTypeId() == Type::Custom) {
						auto srcType = resolveCustomTypeName((CustomTypeNameNode *)src.get());

						switch (srcType->getNodeType()) {
							case NodeType::Class: {
								auto st = std::static_pointer_cast<ClassNode>(srcType);

								do {
									if ((void *)st.get() == (void *)dt.get())
										return true;
									st = std::static_pointer_cast<ClassNode>(resolveCustomTypeName((CustomTypeNameNode *)st->parentClass.get()));

									auto scope = scopeOf(st.get());
									assert(scope);

									if (scope->members.count("operator@" + std::to_string(dest, this)))
										return true;
								} while (st);

								break;
							}
							case NodeType::Interface:
								return _isTypeNamesConvertible(std::static_pointer_cast<InterfaceNode>(srcType), dt);
							case NodeType::Trait:
								return _isTypeNamesConvertible(dt, std::static_pointer_cast<TraitNode>(srcType));
							default:
								throw std::logic_error("Unresolved node type");
						}
					}

					return false;
				}
				case NodeType::Trait: {
					auto dt = std::static_pointer_cast<TraitNode>(destType);
					break;
				}
				case NodeType::Alias: {
					auto dt = std::static_pointer_cast<AliasNode>(destType);
					break;
				}
				case NodeType::GenericParam: {
					return false;
				}
			}
			break;
		}
		case Type::Fn:
			return false;
		case Type::Auto:
		case Type::Void:
			throw std::logic_error("Invalid destination type");
	}

	if (src->getTypeId() == Type::Custom) {
		auto t = std::static_pointer_cast<CustomTypeNameNode>(src);
		auto scope = scopeOf(t.get());

		assert(scope);

		if (scope->members.count("operator@" + std::to_string(dest, this)))
			return true;
		return false;
	}

	return false;
}

std::shared_ptr<AstNode> Compiler::resolveCustomTypeName(CustomTypeNameNode *typeName) {
	if (!typeName->cachedResolvedResult.expired())
		return typeName->cachedResolvedResult.lock();

	// Check if the type refers to a generic parameter.
	if ((typeName->ref.size() == 1) && (typeName->ref[0].genericArgs.empty())) {
		auto genericParam = lookupGenericParam(typeName->scope->owner->shared_from_this(), typeName->ref[0].name);
		if (genericParam) {
#if SLKC_WITH_LANGUAGE_SERVER
			// Update corresponding semantic information.
			auto &tokenInfo = tokenInfos[typeName->ref[0].idxToken];
			tokenInfo.semanticInfo.correspondingMember = genericParam;
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::TypeParam;
#endif

			typeName->cachedResolvedResult = genericParam;
			return genericParam;
		}
	}

	// Check the type with scope where the type name is created.
	if (typeName->scope) {
		std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> resolvedPartsOut;

		if (resolveIdRefWithScope(typeName->scope, typeName->ref, resolvedPartsOut)) {
			if (resolvedPartsOut.size() > 1)
				throw FatalCompilationError(
					Message(
						Location(typeName->getLocation()),
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
						Location(typeName->getLocation()),
						MessageType::Error,
						"Expecting a static identifier"));

			typeName->cachedResolvedResult = resolvedPartsOut.back().second;
			return resolvedPartsOut.back().second;
		}
	}

	// Cannot resolve the type name - generate an error.
	throw FatalCompilationError(
		Message(
			Location(typeName->getLocation()),
			MessageType::Error,
			"Type `" + std::to_string(typeName->ref, this) + "' was not found"));
}

bool Compiler::isSameType(std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y) {
	if (x->getTypeId() != y->getTypeId())
		return false;

	switch (x->getTypeId()) {
		case Type::Custom: {
			std::shared_ptr<AstNode> xDest = resolveCustomTypeName((CustomTypeNameNode *)x.get()),
								yDest = resolveCustomTypeName((CustomTypeNameNode *)y.get());

			return xDest == yDest;
		}
		case Type::Array:
			return isSameType(
				std::static_pointer_cast<ArrayTypeNameNode>(x)->elementType,
				std::static_pointer_cast<ArrayTypeNameNode>(y)->elementType);
		default:
			return true;
	}
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

std::shared_ptr<AstNode> RefTypeNameNode::doDuplicate() {
	return std::make_shared<RefTypeNameNode>(*this);
}

std::shared_ptr<AstNode> slake::slkc::ContextTypeNameNode::doDuplicate() {
	return std::make_shared<ContextTypeNameNode>(*this);
}
