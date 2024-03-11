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
		case TypeId::OBJECT:
			return ((ObjectValue *)value)->scope->getMember(name);
		case TypeId::CLASS:
		case TypeId::INTERFACE:
		case TypeId::TRAIT:
		case TypeId::MOD:
			return ((ModuleValue *)value)->scope->getMember(name);
		case TypeId::VAR:
			return memberOf(((VarValue *)value)->getData(), name);
		case TypeId::ALIAS:
			return memberOf(((AliasValue *)value)->src, name);
		case TypeId::ROOT:
			return ((RootValue *)value)->scope->getMember(name);
		case TypeId::ARRAY:
		case TypeId::MAP:
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
		case TypeId::STRING:
		case TypeId::BOOL:
		case TypeId::FN:
		case TypeId::REF:
		case TypeId::NONE:
		case TypeId::GENERIC_ARG:
		case TypeId::ANY:
		case TypeId::REG_REF:
		case TypeId::LVAR_REF:
		case TypeId::ARG_REF:
		case TypeId::TYPENAME:
		case TypeId::CONTEXT:
			return nullptr;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

std::deque<std::pair<Scope*, MemberValue*>> slake::memberChainOf(Value *value, const std::string &name) {
	if (!value)
		return {};

	switch (auto type = value->getType(); type.typeId) {
		case TypeId::OBJECT:
			return ((ObjectValue *)value)->scope->getMemberChain(name);
		case TypeId::CLASS:
		case TypeId::INTERFACE:
		case TypeId::TRAIT:
		case TypeId::MOD:
			return ((ModuleValue *)value)->scope->getMemberChain(name);
		case TypeId::VAR:
			return memberChainOf(((VarValue *)value)->getData(), name);
		case TypeId::ALIAS:
			return memberChainOf(((AliasValue *)value)->src, name);
		case TypeId::ROOT:
			return ((RootValue *)value)->scope->getMemberChain(name);
		case TypeId::ARRAY:
		case TypeId::MAP:
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
		case TypeId::STRING:
		case TypeId::BOOL:
		case TypeId::FN:
		case TypeId::REF:
		case TypeId::NONE:
		case TypeId::GENERIC_ARG:
		case TypeId::ANY:
		case TypeId::REG_REF:
		case TypeId::LVAR_REF:
		case TypeId::ARG_REF:
		case TypeId::TYPENAME:
		case TypeId::CONTEXT:
			return {};
		default:
			throw std::logic_error("Unhandled value type");
	}
}
