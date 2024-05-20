#include "../compiler.h"

using namespace slake::slkc;

std::string std::to_string(std::shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool forMangling) {
	std::string s = typeName->isConst ? "const " : "";
	switch (typeName->getTypeId()) {
		case TypeId::I8:
			return s + "i8";
		case TypeId::I16:
			return s + "i16";
		case TypeId::I32:
			return s + "i32";
		case TypeId::I64:
			return s + "i64";
		case TypeId::U8:
			return s + "u8";
		case TypeId::U16:
			return s + "u16";
		case TypeId::U32:
			return s + "u32";
		case TypeId::U64:
			return s + "u64";
		case TypeId::F32:
			return s + "f32";
		case TypeId::F64:
			return s + "f64";
		case TypeId::String:
			return s + "std::string";
		case TypeId::Bool:
			return s + "bool";
		case TypeId::Auto:
			return s + "auto";
		case TypeId::Void:
			return s + "void";
		case TypeId::Any:
			return s + "any";
		case TypeId::Array:
			return s + std::to_string(std::static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType, compiler, forMangling) + "[]";
		case TypeId::Fn: {
			auto t = std::static_pointer_cast<FnTypeNameNode>(typeName);
			s += std::to_string(t->returnType, compiler, forMangling) + " -> (";

			for (size_t i = 0; i < t->paramTypes.size(); ++i) {
				if (i)
					s += ", ";
				s += std::to_string(t->paramTypes[i], compiler, forMangling);
			}

			s += ")";
			return s;
		}
		case TypeId::Custom: {
			slake::slkc::IdRef ref;
			auto m = compiler->resolveCustomTypeName((CustomTypeNameNode *)typeName.get());

			switch (m->getNodeType()) {
				case NodeType::GenericParam:
					return (forMangling ? "!" : "") + std::static_pointer_cast<GenericParamNode>(m)->name;
				default:
					compiler->_getFullName((MemberNode *)m.get(), ref);
					return std::to_string(ref, compiler);
			}
		}
		case TypeId::Bad:
			return "<error type>";
		default:
			throw std::logic_error("Unrecognized type");
	}
}
