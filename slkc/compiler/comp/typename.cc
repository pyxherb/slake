#include "../compiler.h"

using namespace slake::slkc;

bool Compiler::isLiteralType(shared_ptr<TypeNameNode> typeName) {
	// stub
	return false;

	switch (typeName->getNodeType()) {
		case TYPE_I8:
		case TYPE_I16:
		case TYPE_I32:
		case TYPE_I64:
		case TYPE_U8:
		case TYPE_U16:
		case TYPE_U32:
		case TYPE_U64:
		case TYPE_F32:
		case TYPE_F64:
		case TYPE_STRING:
		case TYPE_BOOL:
			return true;
		case TYPE_ARRAY: {
			return isLiteralType(static_pointer_cast<ArrayTypeNameNode>(typeName));
		}
		case TYPE_MAP: {
			auto t = static_pointer_cast<MapTypeNameNode>(typeName);
			return isLiteralType(t->keyType) && isLiteralType(t->valueType);
		}
		case TYPE_AUTO:
		case TYPE_VOID:
		case TYPE_ANY:
		case TYPE_FN:
		case TYPE_CUSTOM:
			return false;
		default:
			throw std::logic_error("Unrecognized type");
	}
}

bool Compiler::isNumericType(shared_ptr<TypeNameNode> node) {
	switch (node->getNodeType()) {
		case TYPE_I8:
		case TYPE_I16:
		case TYPE_I32:
		case TYPE_I64:
		case TYPE_U8:
		case TYPE_U16:
		case TYPE_U32:
		case TYPE_U64:
		case TYPE_F32:
		case TYPE_F64:
		case TYPE_CHAR:
		case TYPE_WCHAR:
			return true;
		default:
			return false;
	}
}

bool Compiler::isCompoundType(shared_ptr<TypeNameNode> node) {
	switch (node->getNodeType()) {
		case TYPE_ARRAY:
		case TYPE_MAP:
		case TYPE_FN:
			return true;
		case TYPE_CUSTOM: {
			auto t = static_pointer_cast<CustomTypeNameNode>(node);
			auto dest = resolveCustomType(t);
			switch (dest->getNodeType()) {
				case AST_CLASS:
				case AST_INTERFACE:
				case AST_TRAIT:
					return true;
				case AST_ALIAS:
					return isCompoundType(
						make_shared<CustomTypeNameNode>(
							Location(t->getLocation()),
							t->ref,
							t->isConst));
				default:
					throw FatalCompilationError(
						Message(
							t->ref[0].loc,
							MSG_ERROR,
							"`" + to_string(t->ref) + "' cannot be referenced as a type"));
			}
		}
		default:
			return false;
	}
}

bool Compiler::_areTypesConvertible(shared_ptr<InterfaceNode> st, shared_ptr<ClassNode> dt) {
	for (auto i : dt->implInterfaces) {
		auto interface = static_pointer_cast<InterfaceNode>(resolveCustomType(i));
		assert(interface->getNodeType() == AST_INTERFACE);

		if (interface == st)
			return true;

		for (auto j : interface->parentInterfaces) {
			auto jt = static_pointer_cast<InterfaceNode>(resolveCustomType(j));
			assert(jt->getNodeType() == AST_INTERFACE);

			if (_areTypesConvertible(jt, dt))
				return true;
		}
	}

	return false;
}

bool Compiler::_areTypesConvertible(shared_ptr<InterfaceNode> st, shared_ptr<InterfaceNode> dt) {
	for (auto i : dt->parentInterfaces) {
		auto interface = static_pointer_cast<InterfaceNode>(resolveCustomType(i));
		assert(interface->getNodeType() == AST_INTERFACE);

		if (interface == st)
			return true;

		for (auto j : interface->parentInterfaces) {
			auto jt = static_pointer_cast<InterfaceNode>(resolveCustomType(j));
			assert(jt->getNodeType() == AST_INTERFACE);

			if (_areTypesConvertible(jt, dt))
				return true;
		}
	}

	return false;
}

bool Compiler::_areTypesConvertible(shared_ptr<MemberNode> st, shared_ptr<TraitNode> dt) {
	for (auto i : dt->scope->members) {
		if (!(scopeOf(st)->members.count(i.first)))
			return false;

		auto sm = scopeOf(st)->members.at(i.first);
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
			case AST_VAR: {
				auto smTyped = static_pointer_cast<VarNode>(sm);
				auto dmTyped = static_pointer_cast<VarNode>(dm);

				// The variables must have the same type.
				if (!isSameType(smTyped->type, dmTyped->type))
					return false;

				break;
			}
			case AST_FN: {
				auto smTyped = static_pointer_cast<FnNode>(sm);
				auto dmTyped = static_pointer_cast<FnNode>(dm);

				for (auto di : dmTyped->overloadingRegistries) {
					for (auto si : smTyped->overloadingRegistries) {
						if (!isSameType(di.returnType, si.returnType))
							return false;

						if (di.params.size() != si.params.size())
							return false;

						for (size_t i = 0; i < di.params.size(); ++i) {
							if (!isSameType(si.params[i].type, di.params[i].type))
								return false;
						}
					}
				}
			}
			case AST_CLASS:
			case AST_INTERFACE:
			case AST_TRAIT:
			case AST_MODULE:
			case AST_ALIAS:
				break;
			default:
				throw std::logic_error("Unrecognized member type");
		}
	}

	return false;
}

bool Compiler::areTypesConvertible(shared_ptr<TypeNameNode> src, shared_ptr<TypeNameNode> dest) {
	if (((src->getTypeId()) == dest->getTypeId()) && (!isCompoundType(src)))
		return true;

	switch (dest->getTypeId()) {
		case TYPE_ANY:
			return true;
		case TYPE_I8:
		case TYPE_I16:
		case TYPE_I32:
		case TYPE_I64:
		case TYPE_U8:
		case TYPE_U16:
		case TYPE_U32:
		case TYPE_U64:
		case TYPE_F32:
		case TYPE_F64:
		case TYPE_CHAR:
		case TYPE_WCHAR:
			if (isNumericType(src))
				return true;
			break;
		case TYPE_BOOL:
			return true;
		case TYPE_STRING:
		case TYPE_WSTRING:
		case TYPE_ARRAY:
		case TYPE_MAP:
			break;
		case TYPE_CUSTOM: {
			auto destType = resolveCustomType(static_pointer_cast<CustomTypeNameNode>(dest));

			switch (destType->getNodeType()) {
				case AST_CLASS: {
					auto dt = static_pointer_cast<ClassNode>(destType);

					if (src->getTypeId() == TYPE_CUSTOM) {
						auto srcType = resolveCustomType(static_pointer_cast<CustomTypeNameNode>(src));

						switch (srcType->getNodeType()) {
							case AST_CLASS: {
								auto st = static_pointer_cast<ClassNode>(srcType);

								do {
									if (st == dt)
										return true;
									st = static_pointer_cast<ClassNode>(resolveCustomType(st->parentClass));

									auto scope = scopeOf(st);
									assert(scope);

									if (scope->members.count("operator@" + to_string(dest)))
										return true;
								} while (st);
							}
							case AST_INTERFACE:
								return _areTypesConvertible(static_pointer_cast<InterfaceNode>(srcType), dt);
							case AST_TRAIT:
								return _areTypesConvertible(dt, static_pointer_cast<TraitNode>(srcType));
							default:
								throw std::logic_error("Unresolved node type");
						}
					}

					return false;
				}
				case AST_INTERFACE: {
					auto dt = static_pointer_cast<InterfaceNode>(destType);

					if (src->getTypeId() == TYPE_CUSTOM) {
						auto srcType = resolveCustomType(static_pointer_cast<CustomTypeNameNode>(src));

						switch (srcType->getNodeType()) {
							case AST_CLASS: {
								auto st = static_pointer_cast<ClassNode>(srcType);

								do {
									if ((void *)st.get() == (void *)dt.get())
										return true;
									st = static_pointer_cast<ClassNode>(resolveCustomType(st->parentClass));

									auto scope = scopeOf(st);
									assert(scope);

									if (scope->members.count("operator@" + to_string(dest)))
										return true;
								} while (st);
							}
							case AST_INTERFACE:
								return _areTypesConvertible(static_pointer_cast<InterfaceNode>(srcType), dt);
							case AST_TRAIT:
								return _areTypesConvertible(dt, static_pointer_cast<TraitNode>(srcType));
							default:
								throw std::logic_error("Unresolved node type");
						}
					}

					return false;
				}
				case AST_TRAIT: {
					auto dt = static_pointer_cast<TraitNode>(destType);
				}
				case AST_ALIAS: {
					auto dt = static_pointer_cast<AliasNode>(destType);
				}
			}
			break;
		}
		case TYPE_FN:
			return false;
		case TYPE_AUTO:
		case TYPE_VOID:
			throw std::logic_error("Invalid destination type");
	}

	if (src->getTypeId() == TYPE_CUSTOM) {
		auto t = static_pointer_cast<CustomTypeNameNode>(src);
		auto scope = scopeOf(t);

		assert(scope);

		if (scope->members.count("operator@" + to_string(dest)))
			return true;
		return false;
	}
}

shared_ptr<AstNode> Compiler::resolveCustomType(shared_ptr<CustomTypeNameNode> typeName) {
	if (typeName->resolved)
		return typeName->resolvedDest;

	deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;

	if (resolveRef(typeName->ref, resolvedParts)) {
		typeName->resolvedDest = resolvedParts.back().second;
		typeName->resolved = true;
		return typeName->resolvedDest;
	}

	if (resolvedParts.size() > 1)
		throw FatalCompilationError(
			Message(
				Location(typeName->getLocation()),
				MSG_ERROR,
				"Expecting a static identifier"));

	throw FatalCompilationError(
		Message(
			Location(typeName->getLocation()),
			MSG_ERROR,
			"`" + to_string(typeName->ref) + "' was not found"));
}

bool Compiler::isSameType(shared_ptr<TypeNameNode> x, shared_ptr<TypeNameNode> y) {
	if (x->getTypeId() != y->getTypeId())
		return false;

	switch (x->getTypeId()) {
		case TYPE_CUSTOM: {
			auto xDest = resolveCustomType(static_pointer_cast<CustomTypeNameNode>(x)),
				 yDest = resolveCustomType(static_pointer_cast<CustomTypeNameNode>(y));

			return xDest == yDest;
		}
		case TYPE_MAP:
			return isSameType(
					   static_pointer_cast<MapTypeNameNode>(x)->keyType,
					   static_pointer_cast<MapTypeNameNode>(y)->keyType) &&
				   isSameType(
					   static_pointer_cast<MapTypeNameNode>(x)->valueType,
					   static_pointer_cast<MapTypeNameNode>(y)->valueType);
		case TYPE_ARRAY:
			return isSameType(
				static_pointer_cast<ArrayTypeNameNode>(x)->elementType,
				static_pointer_cast<ArrayTypeNameNode>(y)->elementType);
		default:
			return true;
	}
}
