#include "../compiler.h"

using namespace slake::slkc;

bool Compiler::GenericNodeArgListComparator::operator()(
	const std::deque<std::shared_ptr<TypeNameNode>> &lhs,
	const std::deque<std::shared_ptr<TypeNameNode>> &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return true;
	if (lhs.size() > rhs.size())
		return false;

	for (size_t i = 0; i < lhs.size(); ++i) {
		auto l = lhs[i], r = rhs[i];
		auto lhsTypeId = l->getTypeId(), rhsTypeId = r->getTypeId();

		if (lhsTypeId < rhsTypeId)
			return true;
		else if (lhsTypeId > rhsTypeId)
			return false;
		else {
			//
			// Do some special checks for some kinds of type name - such as custom.
			//
			switch (lhsTypeId) {
				case TypeId::Custom: {
					auto lhsTypeName = std::static_pointer_cast<CustomTypeNameNode>(lhs[i]),
						 rhsTypeName = std::static_pointer_cast<CustomTypeNameNode>(rhs[i]);

					if (lhsTypeName->compiler < rhsTypeName->compiler)
						return true;
					else if (lhsTypeName->compiler > rhsTypeName->compiler)
						return false;
					else {
						std::shared_ptr<AstNode> lhsNode, rhsNode;
						try {
							lhsNode = lhsTypeName->compiler->resolveCustomTypeName(lhsTypeName.get());
							rhsNode = rhsTypeName->compiler->resolveCustomTypeName(rhsTypeName.get());
						} catch (FatalCompilationError e) {
							// Supress the exceptions - the function should be noexcept, we have to raise compilation errors out of the comparator.
						}

						if (lhsNode < rhsNode)
							return true;
						else if (lhsNode > rhsNode)
							return false;
					}

					break;
				}
			}
		}
	}

	return false;
}

void Compiler::walkTypeNameNodeForGenericInstantiation(
	std::shared_ptr<TypeNameNode> &type,
	GenericNodeInstantiationContext &instantiationContext) {
	if (!type)
		return;

	if (type->getTypeId() == TypeId::Custom) {
		auto t = std::static_pointer_cast<CustomTypeNameNode>(type);

		if ((t->ref->entries.size() == 1) &&
			(t->ref->entries[0].genericArgs.empty())) {
			if (auto it = instantiationContext.mappedGenericArgs.find(t->ref->entries[0].name);
				it != instantiationContext.mappedGenericArgs.end()) {
				// Note that we use nullptr to identify irreplaceable generic parameters.
				if (it->second)
					type = it->second;
			}
		} else {
			for (auto &i : t->ref->entries) {
				for (auto &j : i.genericArgs)
					walkTypeNameNodeForGenericInstantiation(j, instantiationContext);
			}
		}
	}
}

void Compiler::walkNodeForGenericInstantiation(
	std::shared_ptr<AstNode> node,
	GenericNodeInstantiationContext &instantiationContext) {
	switch (node->getNodeType()) {
		case NodeType::Fn: {
			std::shared_ptr<FnNode> n = std::static_pointer_cast<FnNode>(node);

			if (auto scope = scopeOf(n->parent); scope)
				scanAndLinkParentFns(scope.get(), n.get(), n->name);

			for (auto &i : n->overloadingRegistries) {
				if (i->genericParams.size() && n != instantiationContext.mappedNode) {
					GenericNodeInstantiationContext newInstantiationContext = instantiationContext;

					// Mark the generic parameters in the overloading as irreplaceable.
					// Note that we use nullptr to identify irreplaceable generic parameters.
					for (size_t j = 0; j < i->genericParams.size(); ++j) {
						newInstantiationContext.mappedGenericArgs[i->genericParams[j]->name] = {};
					}

					walkTypeNameNodeForGenericInstantiation(i->returnType, newInstantiationContext);

					for (auto &j : i->genericParams)
						walkNodeForGenericInstantiation(j, newInstantiationContext);

					for (auto &j : i->params) {
						j->originalType = j->type;
						walkTypeNameNodeForGenericInstantiation(j->type, newInstantiationContext);
					}
				} else {
					walkTypeNameNodeForGenericInstantiation(i->returnType, instantiationContext);

					for (auto &j : i->genericParams)
						walkNodeForGenericInstantiation(j, instantiationContext);

					for (auto &j : i->params) {
						j->originalType = j->type
											  ? j->type->duplicate<TypeNameNode>()
											  : std::make_shared<AnyTypeNameNode>(SIZE_MAX);
						walkTypeNameNodeForGenericInstantiation(j->type, instantiationContext);
					}
				}
			}
			break;
		}
		case NodeType::Var: {
			std::shared_ptr<VarNode> n = std::static_pointer_cast<VarNode>(node);

			walkTypeNameNodeForGenericInstantiation(n->type, instantiationContext);

			break;
		}
		case NodeType::Class: {
			std::shared_ptr<ClassNode> n = std::static_pointer_cast<ClassNode>(node);

			if (n->genericParams.size() && n != instantiationContext.mappedNode) {
				GenericNodeInstantiationContext newInstantiationContext = instantiationContext;

				// Mark the generic parameters in the overloading as irreplaceable.
				// Note that we use nullptr to identify irreplaceable generic parameters.
				for (size_t j = 0; j < n->genericParams.size(); ++j) {
					newInstantiationContext.mappedGenericArgs[n->genericParams[j]->name] = {};
				}

				for (auto &i : n->genericParams)
					walkNodeForGenericInstantiation(i, newInstantiationContext);

				if (n->parentClass)
					walkTypeNameNodeForGenericInstantiation(n->parentClass, newInstantiationContext);

				for (auto &i : n->implInterfaces) {
					walkTypeNameNodeForGenericInstantiation(i, newInstantiationContext);
				}

				for (auto &i : n->scope->members) {
					walkNodeForGenericInstantiation(i.second, newInstantiationContext);
				}
			} else {
				for (auto &i : n->genericParams)
					walkNodeForGenericInstantiation(i, instantiationContext);

				if (n->parentClass)
					walkTypeNameNodeForGenericInstantiation(n->parentClass, instantiationContext);

				for (auto &i : n->implInterfaces) {
					walkTypeNameNodeForGenericInstantiation(i, instantiationContext);
				}

				for (auto &i : n->scope->members) {
					walkNodeForGenericInstantiation(i.second, instantiationContext);
				}
			}

			break;
		}
		case NodeType::Interface: {
			std::shared_ptr<InterfaceNode> n = std::static_pointer_cast<InterfaceNode>(node);

			for (auto &i : n->genericParams)
				walkNodeForGenericInstantiation(i, instantiationContext);

			for (auto &i : n->parentInterfaces) {
				walkTypeNameNodeForGenericInstantiation(i, instantiationContext);
			}

			for (auto &i : n->scope->members) {
				walkNodeForGenericInstantiation(i.second, instantiationContext);
			}

			break;
		}
		case NodeType::GenericParam: {
			std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

			if (n->baseType)
				walkTypeNameNodeForGenericInstantiation(n->baseType, instantiationContext);
			for (auto &i : n->interfaceTypes)
				walkTypeNameNodeForGenericInstantiation(i, instantiationContext);

			break;
		}
	}
}

void Compiler::mapGenericParams(std::shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext) {
	// DO NOT map functions because their generic parameters are saved in overloadings.
	if (node->getNodeType() == NodeType::Fn)
		return;

	if (instantiationContext.genericArgs->size() != node->genericParams.size()) {
		pushMessage(
			curDocName,
			Message(
				tokenRangeToSourceLocation(instantiationContext.genericArgs->at(0)->tokenRange),
				MessageType::Error,
				"Mismatched generic argument number"));

		if (instantiationContext.genericArgs->size() < node->genericParams.size()) {
			for (size_t i = instantiationContext.genericArgs->size(); i < node->genericParams.size(); ++i) {
				instantiationContext.mappedGenericArgs[node->genericParams[i]->name] = std::make_shared<BadTypeNameNode>(SIZE_MAX, SIZE_MAX);
			}
		}
	}

	for (size_t i = 0; i < node->genericParams.size(); ++i) {
		instantiationContext.mappedGenericArgs[node->genericParams[i]->name] = instantiationContext.genericArgs->at(i);
	}
}

std::shared_ptr<MemberNode> Compiler::instantiateGenericNode(std::shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext) {
	if (auto it = _genericCacheDir.find(node.get()); it != _genericCacheDir.end()) {
		if (auto subIt = it->second.find(*instantiationContext.genericArgs); subIt != it->second.end())
			return std::static_pointer_cast<MemberNode>(subIt->second->shared_from_this());
	}

	std::shared_ptr<MemberNode> newInstance = node->duplicate<MemberNode>();

	instantiationContext.mappedNode = newInstance;

	mapGenericParams(node, instantiationContext);
	walkNodeForGenericInstantiation(newInstance, instantiationContext);

	newInstance->genericArgs = *instantiationContext.genericArgs;
	newInstance->originalValue = node.get();

	_genericCacheDir[node.get()][*instantiationContext.genericArgs] = newInstance;

	return newInstance;
}

std::shared_ptr<FnOverloadingNode> Compiler::instantiateGenericFnOverloading(std::shared_ptr<FnOverloadingNode> overloading, GenericNodeInstantiationContext &instantiationContext) {
	mapGenericParams(overloading, instantiationContext);
	walkTypeNameNodeForGenericInstantiation(overloading->returnType, instantiationContext);

	auto newInstance = overloading->duplicate<FnOverloadingNode>();

	for (auto &j : newInstance->genericParams)
		walkNodeForGenericInstantiation(j, instantiationContext);

	for (auto &j : newInstance->params) {
		j->originalType = j->type;
		walkTypeNameNodeForGenericInstantiation(j->type, instantiationContext);
	}

	return newInstance;
}
