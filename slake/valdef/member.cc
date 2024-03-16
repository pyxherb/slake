#include <slake/runtime.h>
#include <slake/type.h>

using namespace slake;

MemberValue::MemberValue(Runtime *rt, AccessModifier access)
	: Value(rt), AccessModified(access) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

MemberValue::~MemberValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

std::string MemberValue::getName() const {
	auto s = _name;
	if (_genericArgs.size()) {
		s += "<";
		for (size_t i = 0; i < _genericArgs.size(); ++i) {
			if (i)
				s += ", ";
			s += std::to_string(_genericArgs[i], _rt);
		}
		s += ">";
	}
	return s;
}

const Value *MemberValue::getParent() const { return _parent; }
Value *MemberValue::getParent() { return _parent; }

void MemberValue::bind(Value *parent, std::string name) {
	_parent = parent, _name = name;
}

void MemberValue::unbind() {
	if (!_parent)
		throw std::logic_error("Unbinding an unbound member value");
	_parent = nullptr;
	_name.clear();
}

void Scope::_getMemberChain(const std::string &name, std::deque<std::pair<Scope*, MemberValue*>> &membersOut) {
	if (auto m = getMember(name); m)
		membersOut.push_back({this, m});

	if (parent)
		parent->_getMemberChain(name, membersOut);
}

Value *slake::memberOf(Value *value, const std::string &name) {
	if (!value)
		return nullptr;

	switch (auto type = value->getType(); type.typeId) {
		case TypeId::Object:
			return ((ObjectValue *)value)->scope->getMember(name);
		case TypeId::Class:
		case TypeId::Interface:
		case TypeId::Trait:
		case TypeId::Module:
			return ((ModuleValue *)value)->scope->getMember(name);
		case TypeId::Var:
			return memberOf(((VarValue *)value)->getData(), name);
		case TypeId::Alias:
			return memberOf(((AliasValue *)value)->src, name);
		case TypeId::RootValue:
			return ((RootValue *)value)->scope->getMember(name);
		case TypeId::Array:
		case TypeId::Map:
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
		case TypeId::Fn:
		case TypeId::Ref:
		case TypeId::None:
		case TypeId::GenericArg:
		case TypeId::Any:
		case TypeId::RegRef:
		case TypeId::LocalVarRef:
		case TypeId::ArgRef:
		case TypeId::TypeName:
		case TypeId::Context:
			return nullptr;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

std::deque<std::pair<Scope*, MemberValue*>> slake::memberChainOf(Value *value, const std::string &name) {
	if (!value)
		return {};

	switch (auto type = value->getType(); type.typeId) {
		case TypeId::Object:
			return ((ObjectValue *)value)->scope->getMemberChain(name);
		case TypeId::Class:
		case TypeId::Interface:
		case TypeId::Trait:
		case TypeId::Module:
			return ((ModuleValue *)value)->scope->getMemberChain(name);
		case TypeId::Var:
			return memberChainOf(((VarValue *)value)->getData(), name);
		case TypeId::Alias:
			return memberChainOf(((AliasValue *)value)->src, name);
		case TypeId::RootValue:
			return ((RootValue *)value)->scope->getMemberChain(name);
		case TypeId::Array:
		case TypeId::Map:
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
		case TypeId::Fn:
		case TypeId::Ref:
		case TypeId::None:
		case TypeId::GenericArg:
		case TypeId::Any:
		case TypeId::RegRef:
		case TypeId::LocalVarRef:
		case TypeId::ArgRef:
		case TypeId::TypeName:
		case TypeId::Context:
			return {};
		default:
			throw std::logic_error("Unhandled value type");
	}
}
