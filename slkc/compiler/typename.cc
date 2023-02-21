#include "scope.hh"

std::string Slake::Compiler::CustomTypeName::toString() const {
	return "@" + std::to_string(*typeRef);
}

/// @brief Check if type 1 and type 2 are the same type.
/// @param t1 Type 1.
/// @param t2 Type 2.
/// @return True if equal, false otherwise.
bool Slake::Compiler::isSameType(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	if (t1->typeName == t2->typeName) {
		switch (t1->typeName) {
			case EvalType::FN: {
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
			case EvalType::CUSTOM: {
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
	if (isSameType(t1, t2))
		return true;
	switch (t1->typeName) {
		case EvalType::I8:
		case EvalType::I16:
		case EvalType::I32:
		case EvalType::I64:
		case EvalType::U8:
		case EvalType::U16:
		case EvalType::U32:
		case EvalType::U64:
		case EvalType::FLOAT:
		case EvalType::DOUBLE:
		case EvalType::BOOL:
			switch (t1->typeName) {
				case EvalType::I8:
				case EvalType::I16:
				case EvalType::I32:
				case EvalType::I64:
				case EvalType::U8:
				case EvalType::U16:
				case EvalType::U32:
				case EvalType::U64:
				case EvalType::FLOAT:
				case EvalType::DOUBLE:
					return true;
				default:
					return false;
			}
		case EvalType::CUSTOM:
			return isBaseOf(t2, t1) || isBaseOf(t1, t2);
	}
	return false;
}

/// @brief Check if a type is base of another one.
/// @param t1 Type to be checked.
/// @param t2 Type to check.
/// @return True if T1 is base class of T2, false otherwise.
bool Slake::Compiler::isBaseOf(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	if (t2->typeName == EvalType::CUSTOM) {
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
						if (ct1->scope.lock()->getType(ct1->typeRef) == t)
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
			case Type::Kind::TRAIT: {
				auto t = std::static_pointer_cast<TraitType>(type2);
				if (type1->getKind()!=Type::Kind::TRAIT)
					return false;
				auto ct1 = std::static_pointer_cast<TraitType>(type1);
				for (auto &i : t->impls->impls) {
					if (ct1 == t)
						return true;
				}
				break;
			}
			case Type::Kind::STRUCT:
				return false;
		}
		return true;
	}
	return false;
}

/// @brief Check if a type was derived from another one.
/// @param t1 Type to be checked.
/// @param t2 Type to check.
/// @return True if T1 was derived from T2, false otherwise.
bool Slake::Compiler::isDerivedFrom(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2) {
	return false;
}
