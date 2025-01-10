#include "cxxast.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::cxxast;

bool ParamListComparator::operator()(const ParamList &lhs, const ParamList &rhs) {
	if (lhs.size() < rhs.size())
		return true;
	if (lhs.size() > rhs.size())
		return false;
	for (size_t i = 0; i < lhs.size(); ++i) {
		const Type &lhsType = lhs.at(i), &rhsType = rhs.at(i);

		if (lhsType < rhsType)
			return true;
		if (lhsType > rhsType)
			return false;
	}
	return false;
}

ASTNode::ASTNode(NodeKind nodeKind) : nodeKind(nodeKind) {
}

ASTNode::~ASTNode() {
}

AbstractMember::AbstractMember(NodeKind nodeKind, std::string &&name) : ASTNode(nodeKind), name(std::move(name)) {
}

AbstractMember::~AbstractMember() {
}

AbstractModule::AbstractModule(NodeKind nodeKind, std::string &&name) : AbstractMember(nodeKind, std::move(name)) {
}

AbstractModule::~AbstractModule() {
}

void AbstractModule::addPublicMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode) {
	removeMember(name);
	publicMembers.insert({ std::move(name), memberNode });
}

void AbstractModule::addProtectedMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode) {
	removeMember(name);
	protectedMembers.insert({ std::move(name), memberNode });
}

std::shared_ptr<AbstractMember> AbstractModule::getMember() {
	if (auto it = publicMembers.find(name); it != publicMembers.end()) {
		return it->second;
	}
	if (auto it = protectedMembers.find(name); it != protectedMembers.end()) {
		return it->second;
	}
}

void AbstractModule::removeMember(const std::string &name) {
	if (auto it = publicMembers.find(name); it != publicMembers.end()) {
		publicMembers.erase(it);
	}
	if (auto it = protectedMembers.find(name); it != protectedMembers.end()) {
		protectedMembers.erase(it);
	}
}

FnOverloading::FnOverloading() : ASTNode(NodeKind::FnOverloading) {
}

FnOverloading::~FnOverloading() {
}

Fn::Fn(std::string &&name) : AbstractMember(NodeKind::Fn, std::move(name)) {
}

Fn::~Fn() {
}

Struct::Struct(std::string &&name) : AbstractModule(NodeKind::Struct, std::move(name)) {
}

Struct::~Struct() {
}

Class::Class(std::string &&name) : AbstractModule(NodeKind::Class, std::move(name)) {
}

Class::~Class() {
}

Namespace::Namespace(std::string &&name) : AbstractModule(NodeKind::Namespace, std::move(name)) {
}

Namespace::~Namespace() {
}
