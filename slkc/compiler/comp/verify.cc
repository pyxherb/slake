#include "../compiler.h"

using namespace slake::slkc;

static void _throwCyclicInheritanceError(const SourceLocation &loc) {
	throw FatalCompilationError(
		Message(
			loc,
			MessageType::Error,
			"Cyclic inheritance detected"));
}

void Compiler::verifyInheritanceChain(ClassNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	if (node->parentClass) {
#if SLKC_WITH_LANGUAGE_SERVER
		updateCompletionContext(node->parentClass, CompletionContext::Type);
#endif

		if (node->parentClass->getTypeId() != TypeId::Custom)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(node->parentClass->tokenRange),
					MessageType::Error,
					"The type cannot be inherited"));

		auto cls = resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());

		if (cls->getNodeType() != NodeType::Class)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(node->parentClass->tokenRange),
					MessageType::Error,
					"`" + std::to_string(node->parentClass, this) + "' is not a class"));

		auto m = std::static_pointer_cast<ClassNode>(cls);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(tokenRangeToSourceLocation(node->parentClass->tokenRange));

		verifyInheritanceChain(m->originalValue ? (ClassNode *)m->originalValue : m.get(), walkedNodes);
	}

	for (auto &i : node->implInterfaces) {
#if SLKC_WITH_LANGUAGE_SERVER
		updateCompletionContext(i, CompletionContext::Type);
#endif

		if (i->getTypeId() != TypeId::Custom)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(i->tokenRange),
					MessageType::Error,
					"The type cannot be implemented"));

		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(i->tokenRange),
					MessageType::Error,
					"`" + std::to_string(i, this) + "' is not an interface"));

		auto m = std::static_pointer_cast<InterfaceNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(tokenRangeToSourceLocation(i->tokenRange));

		verifyInheritanceChain(m->originalValue ? (InterfaceNode *)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(InterfaceNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	for (auto &i : node->parentInterfaces) {
#if SLKC_WITH_LANGUAGE_SERVER
		updateCompletionContext(i, CompletionContext::Type);
#endif

		if (i->getTypeId() != TypeId::Custom)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(i->tokenRange),
					MessageType::Error,
					"Specified type cannot be implemented"));

		auto parent = resolveCustomTypeName((CustomTypeNameNode *)i.get());

		if (parent->getNodeType() != NodeType::Interface)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(i->tokenRange),
					MessageType::Error,
					"`" + std::to_string(i, this) + "' is not an interface"));

		auto m = std::static_pointer_cast<InterfaceNode>(parent);

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(tokenRangeToSourceLocation(i->tokenRange));

		verifyInheritanceChain(m->originalValue ? (InterfaceNode *)m->originalValue : m.get(), walkedNodes);
	}
}

void Compiler::verifyInheritanceChain(GenericParamNode *node, std::set<AstNode *> &walkedNodes) {
	walkedNodes.insert(node);

	if (node->baseType) {
		auto typeName = node->baseType;
		if (typeName->getTypeId() != TypeId::Custom)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(typeName->tokenRange),
					MessageType::Error,
					"The type cannot be inherited"));

		auto t = std::static_pointer_cast<CustomTypeNameNode>(typeName);
		auto m = resolveCustomTypeName(t.get());

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(tokenRangeToSourceLocation(t->tokenRange));

		if (m->getNodeType() == NodeType::GenericParam)
			verifyInheritanceChain((GenericParamNode*)m.get(), walkedNodes);
	}

	for (auto &i : node->interfaceTypes) {
		if (i->getTypeId() != TypeId::Custom)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(i->tokenRange),
					MessageType::Error,
					"The type cannot be implemented"));

		auto t = std::static_pointer_cast<CustomTypeNameNode>(i);
		auto m = resolveCustomTypeName(t.get());

		if (walkedNodes.count(m.get()))
			_throwCyclicInheritanceError(tokenRangeToSourceLocation(t->tokenRange));

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
			if (typeName->getTypeId() != TypeId::Custom)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(typeName->tokenRange),
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
			if (j->getTypeId() != TypeId::Custom)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(j->tokenRange),
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
