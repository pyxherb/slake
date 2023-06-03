#include <slkparse.hh>

std::string Slake::Compiler::CustomTypeName::toString() const {
	return "@" + std::to_string(*typeRef);
}

/// @brief Get resolved type registry.
/// @return Resolved type registry, returning nullptr is ill-formed (the existence should be checked previously).
std::shared_ptr<Slake::Compiler::Type> Slake::Compiler::CustomTypeName::resolveType() {
	if (!_cachedType.expired())
		return _cachedType.lock();
	auto result = scope.lock()->getType(typeRef);
	if (result)
		_cachedType = result;
	return result;
}

bool Slake::Compiler::isLiteralType(std::shared_ptr<TypeName> t) {
	switch (t->kind) {
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
		case TypeNameKind::STRING:
		case TypeNameKind::BOOL:
		case TypeNameKind::ARRAY:
		case TypeNameKind::MAP:
			return true;
		default:
			return false;
	}
}

bool Slake::Compiler::isScalarType(std::shared_ptr<TypeName> t) {
	switch (t->kind) {
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
		case TypeNameKind::STRING:
		case TypeNameKind::BOOL:
			return true;
		default:
			return false;
	}
}

bool Slake::Compiler::isIntegralType(std::shared_ptr<TypeName> t) {
	switch (t->kind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64:
		case TypeNameKind::BOOL:
			return true;
		default:
			return false;
	}
}

bool Slake::Compiler::isSignedType(std::shared_ptr<TypeName> t) {
	switch (t->kind) {
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::F32:
		case TypeNameKind::F64:
			return true;
		default:
			return false;
	}
}

bool Slake::Compiler::isFloatingPointType(std::shared_ptr<TypeName> t) {
	switch (t->kind) {
		case TypeNameKind::F32:
		case TypeNameKind::F64:
			return true;
		default:
			return false;
	}
}

bool Slake::Compiler::isArithmeticType(std::shared_ptr<TypeName> t) {
	switch (t->kind) {
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
		case TypeNameKind::STRING:
		case TypeNameKind::BOOL:
			return true;
		default:
			return false;
	}
}

/// @brief Check if type 1 and type 2 are the same type.
/// @param t1 Type 1.
/// @param t2 Type 2.
/// @return True if equal, false otherwise.
bool Slake::Compiler::isSameType(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	if (t1->kind == t2->kind) {
		switch (t1->kind) {
			case TypeNameKind::FN: {
				auto fnType1 = std::static_pointer_cast<FnTypeName>(t1), fnType2 = std::static_pointer_cast<FnTypeName>(t2);
				if (fnType1->resultType != fnType2->resultType)
					return false;
				if (fnType1->argTypes.size() != fnType2->argTypes.size())
					return false;
				for (auto i1 = fnType1->argTypes.begin(), i2 = fnType2->argTypes.begin(); i1 != fnType1->argTypes.end(); i1++, i2++) {
					if (!isSameType((*i1), (*i2)))
						return false;
				}
			}
			case TypeNameKind::CUSTOM: {
				auto type1 = std::static_pointer_cast<CustomTypeName>(t1), type2 = std::static_pointer_cast<CustomTypeName>(t2);
				return type1->scope.lock()->getType(type1->typeRef) == type2->scope.lock()->getType(type2->typeRef);
			}
			default:
				return true;
		}
	}
	return false;
}

/// @brief Check if type 1 can be converted to type 2.
/// @param t1 Type 1.
/// @param t2 Type 2.
/// @return True if convertible, false otherwise.
bool Slake::Compiler::isConvertible(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	if (isSameType(t1, t2) || t1->kind == TypeNameKind::NONE || t2->kind == TypeNameKind::NONE)
		return true;
	switch (t1->kind) {
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
		case TypeNameKind::BOOL:
			switch (t2->kind) {
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
				case TypeNameKind::BOOL:
					return true;
				default:
					return false;
			}
		case TypeNameKind::CUSTOM:
			return isBaseOf(t2, t1) || isBaseOf(t1, t2);
		default:
			return false;
	}
}

/// @brief Check if a type is base of another one.
/// @param t1 Type to be checked.
/// @param t2 Type to check.
/// @return True if T1 is base class of T2, false otherwise.
bool Slake::Compiler::isBaseOf(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	if (t1->kind != TypeNameKind::CUSTOM)
		return false;
	if (t2->kind == TypeNameKind::CUSTOM) {
		auto type1 = std::static_pointer_cast<CustomTypeName>(t1)->scope.lock()->getType(std::static_pointer_cast<CustomTypeName>(t1)->typeRef),
			 type2 = std::static_pointer_cast<CustomTypeName>(t2)->scope.lock()->getType(std::static_pointer_cast<CustomTypeName>(t2)->typeRef);
		switch (type2->getKind()) {
			case Type::Kind::CLASS: {
				auto t = std::static_pointer_cast<ClassType>(type2);
				auto ct1 = std::static_pointer_cast<CustomTypeName>(t1);

				// Check if the type 1 is parent of type 2.
				if (!isBaseOf(t1, t->parent)) {
					// If not, check if type 1 was implemented by type2.
					for (auto &i : t->impls->impls) {
						if (i->kind != TypeNameKind::CUSTOM)
							throw std::logic_error("Only custom types can be derived");
						auto itn = std::static_pointer_cast<CustomTypeName>(i);
						if (ct1->scope.lock()->getType(ct1->typeRef) == itn->scope.lock()->getType(itn->typeRef))
							return true;
					}
					return false;
				}
				return true;
			}
			case Type::Kind::ENUM: {
				auto t = std::static_pointer_cast<EnumType>(type1);
				return isSameType(t->typeName, t1);
			}
			case Type::Kind::INTERFACE: {
				if (type1->getKind() != Type::Kind::INTERFACE)
					return false;
				auto ct1 = std::static_pointer_cast<InterfaceType>(type1), ct2 = std::static_pointer_cast<InterfaceType>(type2);
				if (ct1 == ct2 || ct1->parent == t2)
					return true;
				return isBaseOf(ct1->parent, t2);
				break;
			}
			case Type::Kind::STRUCT:
				return false;
			default:
				return true;
		}
	}
	return false;
}

/// @brief Check if a type was derived from another one.
/// @param t1 Type to be checked.
/// @param t2 Type to check.
/// @return True if T1 was derived from T2, false otherwise.
bool Slake::Compiler::isDerivedFrom(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	return isBaseOf(t2, t1);
}

bool Slake::Compiler::hasOperator(std::shared_ptr<TypeName> t, UnaryOp op) {
	switch (t->kind) {
		case TypeNameKind::CUSTOM: {
			auto tn = std::static_pointer_cast<CustomTypeName>(t)->resolveType();

			switch (tn->getKind()) {
				case Type::Kind::CLASS: {
					auto ct = std::static_pointer_cast<ClassType>(tn);
					if (ct->getScope()->getFn(std::to_string(op)))
						return true;
				}
				case Type::Kind::INTERFACE: {
					auto ct = std::static_pointer_cast<InterfaceType>(tn);
					if (ct->getScope()->getFn(std::to_string(op)))
						return true;
				}
				default:
					return false;
			}
		}
		case TypeNameKind::I8:
		case TypeNameKind::I16:
		case TypeNameKind::I32:
		case TypeNameKind::I64:
		case TypeNameKind::U8:
		case TypeNameKind::U16:
		case TypeNameKind::U32:
		case TypeNameKind::U64: {
			switch (op) {
				case UnaryOp::REV:
					return true;
				default:;
			}
		}
		case TypeNameKind::F32:
		case TypeNameKind::F64: {
			switch (op) {
				case UnaryOp::INC_F:
				case UnaryOp::INC_B:
				case UnaryOp::DEC_F:
				case UnaryOp::DEC_B:
				case UnaryOp::NEG:
				case UnaryOp::NOT:
					return true;
				default:
					return false;
			}
			break;
		}
		default:
			return false;
	}
}

bool Slake::Compiler::hasOperator(std::shared_ptr<TypeName> t, BinaryOp op) {
	if (t->kind != TypeNameKind::CUSTOM)
		return false;

	switch (t->kind) {
		case TypeNameKind::ANY:
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
			return true;
		case TypeNameKind::BOOL: {
			switch (op) {
				case BinaryOp::LAND:
				case BinaryOp::LOR:
				case BinaryOp::LSH:
				case BinaryOp::LSH_ASSIGN:
				case BinaryOp::RSH:
				case BinaryOp::RSH_ASSIGN:
				case BinaryOp::EQ:
				case BinaryOp::NEQ:
				case BinaryOp::ASSIGN:
					return true;
				default:
					return false;
			}
		}
		case TypeNameKind::STRING:
			switch (op) {
				case BinaryOp::ADD:
				case BinaryOp::ADD_ASSIGN:
				case BinaryOp::LAND:
				case BinaryOp::LOR:
				case BinaryOp::EQ:
				case BinaryOp::NEQ:
				case BinaryOp::ASSIGN:
					return true;
				default:
					return false;
			}
		case TypeNameKind::CUSTOM: {
			auto tn = std::static_pointer_cast<CustomTypeName>(t)->resolveType();

			switch (tn->getKind()) {
				case Type::Kind::CLASS: {
					auto ct = std::static_pointer_cast<ClassType>(tn);
					if (ct->getScope()->getFn(std::to_string(op)))
						return true;
				}
				case Type::Kind::INTERFACE: {
					auto ct = std::static_pointer_cast<InterfaceType>(tn);
					if (ct->getScope()->getFn(std::to_string(op)))
						return true;
				}
				default:
					return false;
			}
		}
		default:
			return false;
	}
	return false;
}
