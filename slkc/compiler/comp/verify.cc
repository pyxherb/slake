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
		updateCompletionContext(node->parentClass, CompletionContext::Type);

		if (node->parentClass->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					node->parentClass->getLocation(),
					MessageType::Error,
					"The type cannot be inherited"));

		auto cls = resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());

		if (cls->getNodeType() != NodeType::Class)
			throw FatalCompilationError(
				Message(
					node->parentClass->getLocation(),
					MessageType::Error,
					"`" + std::to_string(node->parentClass, this) + "' is not a class"));

		auto m = std::static_pointer_cast<ClassNode>(cls);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(node->parentClass->getLocation());

		verifyInheritanceChain(m->originalValue ? (ClassNode *)m->originalValue : m.get(), walkedNodes);
	}

	for (auto &i : node->implInterfaces) {
		updateCompletionContext(i, CompletionContext::Type);

		if (i->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"The type cannot be implemented"));

		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + std::to_string(i, this) + "' is not an interface"));

		auto m = std::static_pointer_cast<InterfaceNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(i->getLocation());

		verifyInheritanceChain(m->originalValue ? (InterfaceNode *)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(InterfaceNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	for (auto &i : node->parentInterfaces) {
		updateCompletionContext(i, CompletionContext::Type);

		if (i->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"Specified type cannot be implemented"));

		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + std::to_string(i, this) + "' is not an interface"));

		auto m = std::static_pointer_cast<InterfaceNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(i->getLocation());

		verifyInheritanceChain(m->originalValue ? (InterfaceNode *)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(TraitNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	for (auto &i : node->parentTraits) {
		updateCompletionContext(i, CompletionContext::Type);

		if (i->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + std::to_string(i, this) + "' cannot be implemented"));

		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Trait)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"`" + std::to_string(i, this) + "' is not a trait"));

		auto m = std::static_pointer_cast<TraitNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(i->getLocation());

		verifyInheritanceChain(m->originalValue ? (TraitNode *)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(GenericParamNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	if (node->baseType) {
		auto typeName = node->baseType;
		if (typeName->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					typeName->getLocation(),
					MessageType::Error,
					"The type cannot be inherited"));

		auto t = std::static_pointer_cast<CustomTypeNameNode>(typeName);
		auto m = resolveCustomTypeName(t.get());

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(t->getLocation());

		if (m->getNodeType() == NodeType::GenericParam)
			verifyInheritanceChain((GenericParamNode*)m.get(), walkedNodes);
	}

	for (auto &i : node->interfaceTypes) {
		if (i->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"The type cannot be implemented"));

		auto t = std::static_pointer_cast<CustomTypeNameNode>(i);
		auto m = resolveCustomTypeName(t.get());

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(t->getLocation());

		if (m->getNodeType() == NodeType::GenericParam)
			verifyInheritanceChain((GenericParamNode *)m.get(), walkedNodes);
	}

	for (auto &i : node->traitTypes) {
		if (i->getTypeId() != Type::Custom)
			throw FatalCompilationError(
				Message(
					i->getLocation(),
					MessageType::Error,
					"The type cannot be implemented"));

		auto t = std::static_pointer_cast<CustomTypeNameNode>(i);
		auto m = resolveCustomTypeName(t.get());

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(t->getLocation());

		if (m->getNodeType() == NodeType::GenericParam)
			verifyInheritanceChain((GenericParamNode *)m.get(), walkedNodes);
	}
}

void Compiler::verifyGenericParams(const GenericParamNodeList &params) {
	auto indices = genGenericParamIndicies(params);

	auto verifySingleParam = [this, &indices](std::shared_ptr<TypeNameNode> typeName) {
		{
			auto t = std::static_pointer_cast<CustomTypeNameNode>(typeName);
			auto dest = resolveCustomTypeName(t.get());

			if (dest->getNodeType() == NodeType::GenericParam) {
				auto d = std::static_pointer_cast<GenericParamNode>(dest);

				verifyInheritanceChain(d.get());
			}
		}
	};

	for (auto &i : params) {
		if (i->baseType) {
			auto typeName = i->baseType;
			if (typeName->getTypeId() != Type::Custom)
				throw FatalCompilationError(
					Message(
						typeName->getLocation(),
						MessageType::Error,
						"The type cannot be inherited"));

			auto t = std::static_pointer_cast<CustomTypeNameNode>(typeName);
			auto dest = resolveCustomTypeName(t.get());

			if (dest->getNodeType() == NodeType::GenericParam) {
				auto d = std::static_pointer_cast<GenericParamNode>(dest);

				verifyInheritanceChain(d.get());
			}
		}

		for (auto &j : i->interfaceTypes) {
			if (j->getTypeId() != Type::Custom)
				throw FatalCompilationError(
					Message(
						j->getLocation(),
						MessageType::Error,
						"The type cannot be implemented"));

			auto t = std::static_pointer_cast<CustomTypeNameNode>(j);
			auto dest = resolveCustomTypeName(t.get());

			if (dest->getNodeType() == NodeType::GenericParam) {
				auto d = std::static_pointer_cast<GenericParamNode>(dest);

				verifyInheritanceChain(d.get());
			}
		}

		for (auto &j : i->traitTypes) {
			if (j->getTypeId() != Type::Custom)
				throw FatalCompilationError(
					Message(
						j->getLocation(),
						MessageType::Error,
						"The type cannot be implemented"));

			auto t = std::static_pointer_cast<CustomTypeNameNode>(j);
			auto dest = resolveCustomTypeName(t.get());

			if (dest->getNodeType() == NodeType::GenericParam) {
				auto d = std::static_pointer_cast<GenericParamNode>(dest);

				verifyInheritanceChain(d.get());
			}
		}
	}
}