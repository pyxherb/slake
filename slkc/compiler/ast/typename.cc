#include "../compiler.h"

using namespace slake::slkc;

std::string std::to_string(std::shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool forMangling) {
	std::string s;
	switch (typeName->getTypeId()) {
		case TypeId::I8:
			s = "i8";
			break;
		case TypeId::I16:
			s = "i16";
			break;
		case TypeId::I32:
			s = "i32";
			break;
		case TypeId::I64:
			s = "i64";
			break;
		case TypeId::U8:
			s = "u8";
			break;
		case TypeId::U16:
			s = "u16";
			break;
		case TypeId::U32:
			s = "u32";
			break;
		case TypeId::U64:
			s = "u64";
			break;
		case TypeId::F32:
			s = "f32";
			break;
		case TypeId::F64:
			s = "f64";
			break;
		case TypeId::String:
			s = "string";
			break;
		case TypeId::Bool:
			s = "bool";
			break;
		case TypeId::Auto:
			s = "auto";
			break;
		case TypeId::Void:
			s = "void";
			break;
		case TypeId::Any:
			s = "any";
			break;
		case TypeId::Array:
			s = std::to_string(std::static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType, compiler, forMangling) + "[]";
			break;
		case TypeId::Fn: {
			auto t = std::static_pointer_cast<FnTypeNameNode>(typeName);
			s += std::to_string(t->returnType, compiler, forMangling) + " -> (";

			for (size_t i = 0; i < t->paramTypes.size(); ++i) {
				if (i)
					s += ", ";
				s += std::to_string(t->paramTypes[i], compiler, forMangling);
			}

			s += ")";
			break;
		}
		case TypeId::Custom: {
			std::shared_ptr<IdRefNode> ref = std::make_shared<IdRefNode>();
			auto m = compiler->resolveCustomTypeName((CustomTypeNameNode *)typeName.get());

			switch (m->getNodeType()) {
				case NodeType::GenericParam:
					s = (forMangling ? "!" : "") + std::static_pointer_cast<GenericParamNode>(m)->name;
					break;
				default:
					compiler->_getFullName((MemberNode *)m.get(), ref->entries);
					s = std::to_string(ref->entries, compiler, forMangling);
			}
			break;
		}
		case TypeId::Bad:
			s = "<error type>";
			break;
		default:
			throw std::logic_error("Unrecognized type");
	}

	if (typeName->isRef)
		s += "&";

	return s;
}
