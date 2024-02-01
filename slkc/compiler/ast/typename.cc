#include "../compiler.h"

using namespace slake::slkc;

string std::to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName) {
	string s = typeName->isConst ? "const " : "";
	switch (typeName->getTypeId()) {
		case TYPE_I8:
			return s + "i8";
		case TYPE_I16:
			return s + "i16";
		case TYPE_I32:
			return s + "i32";
		case TYPE_I64:
			return s + "i64";
		case TYPE_U8:
			return s + "u8";
		case TYPE_U16:
			return s + "u16";
		case TYPE_U32:
			return s + "u32";
		case TYPE_U64:
			return s + "u64";
		case TYPE_F32:
			return s + "f32";
		case TYPE_F64:
			return s + "f64";
		case TYPE_STRING:
			return s + "string";
		case TYPE_BOOL:
			return s + "bool";
		case TYPE_AUTO:
			return s + "auto";
		case TYPE_VOID:
			return s + "void";
		case TYPE_ANY:
			return s + "any";
		case TYPE_ARRAY:
			return s + to_string(static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType, compiler, asOperatorName) + "[]";
		case TYPE_MAP: {
			auto t = static_pointer_cast<MapTypeNameNode>(typeName);
			return s + to_string(t->keyType, compiler, asOperatorName) + "[" + to_string(t->valueType, compiler, asOperatorName) + "]";
		}
		case TYPE_FN: {
			auto t = static_pointer_cast<FnTypeNameNode>(typeName);
			s += to_string(t->returnType, compiler, asOperatorName) + " -> (";

			for (size_t i = 0; i < t->paramTypes.size(); ++i) {
				if (i)
					s += ", ";
				s += to_string(t->paramTypes[i], compiler, asOperatorName);
			}

			s += ")";
			return s;
		}
		case TYPE_CUSTOM: {
			slake::slkc::Ref ref;
			compiler->_getFullName((MemberNode *)compiler->resolveCustomType(static_pointer_cast<CustomTypeNameNode>(typeName)).get(), ref);
			return (asOperatorName ? "" : "@") + to_string(ref, compiler);
		}
		default:
			throw std::logic_error("Unrecognized type");
	}
}
