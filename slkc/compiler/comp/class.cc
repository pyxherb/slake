#include "../compiler.h"

using namespace slake::slkc;

void Compiler::getMemberNodes(Scope *scope, std::unordered_map<std::string, MemberNode *> &membersOut) {
	for (auto &i : scope->members) {
		if (!membersOut.count(i.first))
			membersOut[i.first] = i.second.get();
	}
}

void Compiler::getMemberNodes(ClassNode *node, std::unordered_map<std::string, MemberNode *> &membersOut) {
	getMemberNodes(node->scope.get(), membersOut);

	if (node->parentClass) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());

		switch (parent->getNodeType()) {
			case NodeType::Class:
				getMemberNodes((ClassNode *)parent.get(), membersOut);
				break;
			default:
				throw FatalCompilationError(
					Message(
						parent->getLocation(),
						MessageType::Error,
						"`" + to_string(node->parentClass, this) + "' is not a class"));
		}
	}

	for (auto &i : node->implInterfaces) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		switch (parent->getNodeType()) {
			case NodeType::Interface:
				getMemberNodes((InterfaceNode *)parent.get(), membersOut);
				break;
			default:
				throw FatalCompilationError(
					Message(
						parent->getLocation(),
						MessageType::Error,
						"`" + to_string(i, this) + "' is not an interface"));
		}
	}
}

void Compiler::getMemberNodes(InterfaceNode *node, std::unordered_map<std::string, MemberNode *> &membersOut) {
	getMemberNodes(node->scope.get(), membersOut);

	for (auto &i : node->parentInterfaces) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		switch (parent->getNodeType()) {
			case NodeType::Interface:
				getMemberNodes((InterfaceNode *)parent.get(), membersOut);
				break;
			default:
				throw FatalCompilationError(
					Message(
						parent->getLocation(),
						MessageType::Error,
						"`" + to_string(i, this) + "' is not an interface"));
		}
	}
}

void Compiler::getMemberNodes(TraitNode *node, std::unordered_map<std::string, MemberNode *> &membersOut) {
	getMemberNodes(node->scope.get(), membersOut);

	for (auto &i : node->parentTraits) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());
		
		switch (parent->getNodeType()) {
			case NodeType::Trait:
				getMemberNodes((TraitNode *)parent.get(), membersOut);
				break;
			default:
				throw FatalCompilationError(
					Message(
						parent->getLocation(),
						MessageType::Error,
						"`" + to_string(i, this) + "' is not an interface"));
		}
	}
}

void Compiler::verifyInheritanceChain(ClassNode *node, std::set<AstNode *> &walkedNodes) {
	if (walkedNodes.count(node))
		throw FatalCompilationError(
			Message(
				node->getLocation(),
				MessageType::Error,
				"A class cannot be derived from itself"));

	walkedNodes.insert(node);

	if (node->parentClass) {
		auto cls = resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());

		if (cls->getNodeType() != NodeType::Class)
			throw FatalCompilationError(
				Message(
					cls->getLocation(),
					MessageType::Error,
					"`" + to_string(node->parentClass, this) + "' is not a class"));

		verifyInheritanceChain((ClassNode *)cls.get(), walkedNodes);
	}

	for (auto &i : node->implInterfaces) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					parent->getLocation(),
					MessageType::Error,
					"`" + to_string(i, this) + "' is not an interface"));

		verifyInheritanceChain((InterfaceNode *)i.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(InterfaceNode *node, std::set<AstNode *> &walkedNodes) {
	if (walkedNodes.count(node))
		throw FatalCompilationError(
			Message(
				node->getLocation(),
				MessageType::Error,
				"An interface cannot be derived from itself"));

	walkedNodes.insert(node);

	for (auto &i : node->parentInterfaces) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					parent->getLocation(),
					MessageType::Error,
					"`" + to_string(i, this) + "' is not an interface"));

		verifyInheritanceChain((InterfaceNode *)i.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(TraitNode *node, std::set<AstNode *> &walkedNodes) {
	if (walkedNodes.count(node))
		throw FatalCompilationError(
			Message(
				node->getLocation(),
				MessageType::Error,
				"A trait cannot be derived from itself"));

	walkedNodes.insert(node);

	for (auto &i : node->parentTraits) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Trait)
			throw FatalCompilationError(
				Message(
					parent->getLocation(),
					MessageType::Error,
					"`" + to_string(i, this) + "' is not a trait"));

		verifyInheritanceChain((InterfaceNode *)i.get(), walkedNodes);
	}
}
