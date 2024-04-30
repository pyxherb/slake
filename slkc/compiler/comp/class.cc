#include "../compiler.h"

using namespace slake::slkc;

static void _throwCyclicInheritanceError(const Location &loc) {
	throw FatalCompilationError(
		Message(
			loc,
			MessageType::Error,
			"Cyclic inheritance detected"));
}

void Compiler::verifyInheritanceChain(ClassNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	if (node->parentClass) {
		auto cls = resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());

		if (cls->getNodeType() != NodeType::Class)
			throw FatalCompilationError(
				Message(
					node->parentClass->getLocation(),
					MessageType::Error,
					"`" + to_string(node->parentClass, this) + "' is not a class"));

		auto m = static_pointer_cast<ClassNode>(cls);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(node->parentClass->getLocation());

		verifyInheritanceChain(m->originalValue ? (ClassNode *)m->originalValue : m.get(), walkedNodes);
	}

	for (auto &i : node->implInterfaces) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + to_string(i, this) + "' is not an interface"));

		auto m = static_pointer_cast<InterfaceNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(i->getLocation());

		verifyInheritanceChain(m->originalValue ? (InterfaceNode *)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(InterfaceNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	for (auto &i : node->parentInterfaces) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + to_string(i, this) + "' is not an interface"));

		auto m = static_pointer_cast<InterfaceNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(i->getLocation());

		verifyInheritanceChain(m->originalValue ? (InterfaceNode*)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(TraitNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	for (auto &i : node->parentTraits) {
		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Trait)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + to_string(i, this) + "' is not a trait"));

		auto m = static_pointer_cast<TraitNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(i->getLocation());

		verifyInheritanceChain(m->originalValue ? (TraitNode *)m->originalValue : m.get(), walkedNodes);
	}
}
