#include "../compiler.h"

using namespace slake::slkc;

void Compiler::walkTypeNameNodeForGenericInstantiation(
	shared_ptr<TypeNameNode> &type,
	GenericNodeInstantiationContext &instantiationContext) {
	if (type->getTypeId() == Type::Custom) {
		auto t = static_pointer_cast<CustomTypeNameNode>(type);

		if ((t->ref.size() == 1) &&
			(t->ref[0].genericArgs.empty())) {
			if (auto it = instantiationContext.mappedGenericArgs.find(t->ref[0].name);
				it != instantiationContext.mappedGenericArgs.end()) {
				// Note that we use nullptr to identify irreplaceable generic parameters.
				if (it->second)
					type = it->second;
			}
		} else {
			for (auto &i : t->ref) {
				for (auto &j : i.genericArgs)
					walkTypeNameNodeForGenericInstantiation(j, instantiationContext);
			}
		}
	}
}

void Compiler::walkNodeForGenericInstantiation(
	shared_ptr<AstNode> node,
	GenericNodeInstantiationContext &instantiationContext) {
	switch (node->getNodeType()) {
		case NodeType::Fn: {
			shared_ptr<FnNode> n = static_pointer_cast<FnNode>(node);

			for (auto &i : n->overloadingRegistries) {
				if (i->genericParams.empty()) {
					walkTypeNameNodeForGenericInstantiation(i->returnType, instantiationContext);

					for (auto &j : i->genericParams)
						walkNodeForGenericInstantiation(j, instantiationContext);

					for (auto &j : i->params) {
						j.originalType = j.type;
						walkTypeNameNodeForGenericInstantiation(j.type, instantiationContext);
					}
				} else {
					GenericNodeInstantiationContext newInstantiationContext = instantiationContext;

					// Mark the generic parameters in the overloading as irreplaceable.
					// Note that we use nullptr to identify irreplaceable generic parameters.
					for (size_t j = 0; j < i->genericParams.size(); ++j) {
						instantiationContext.mappedGenericArgs[i->genericParams[j]->name] = {};
					}

					walkTypeNameNodeForGenericInstantiation(i->returnType, newInstantiationContext);

					for (auto &j : i->genericParams)
						walkNodeForGenericInstantiation(j, newInstantiationContext);

					for (auto &j : i->params) {
						j.originalType = j.type;
						walkTypeNameNodeForGenericInstantiation(j.type, newInstantiationContext);
					}
				}
			}
			break;
		}
		case NodeType::Var: {
			shared_ptr<VarNode> n = static_pointer_cast<VarNode>(node);

			walkTypeNameNodeForGenericInstantiation(n->type, instantiationContext);

			break;
		}
		case NodeType::Class: {
			shared_ptr<ClassNode> n = static_pointer_cast<ClassNode>(node);

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

			break;
		}
		case NodeType::Interface: {
			shared_ptr<InterfaceNode> n = static_pointer_cast<InterfaceNode>(node);

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
		case NodeType::Trait: {
			shared_ptr<TraitNode> n = static_pointer_cast<TraitNode>(node);

			for (auto &i : n->genericParams)
				walkNodeForGenericInstantiation(i, instantiationContext);

			for (auto &i : n->parentTraits) {
				walkTypeNameNodeForGenericInstantiation(i, instantiationContext);
			}

			for (auto &i : n->scope->members) {
				walkNodeForGenericInstantiation(i.second, instantiationContext);
			}

			break;
		}
		case NodeType::GenericParam: {
			shared_ptr<GenericParamNode> n = static_pointer_cast<GenericParamNode>(node);

			if (n->baseType)
				walkTypeNameNodeForGenericInstantiation(n->baseType, instantiationContext);
			for (auto &i : n->interfaceTypes)
				walkTypeNameNodeForGenericInstantiation(i, instantiationContext);
			for (auto &i : n->traitTypes)
				walkTypeNameNodeForGenericInstantiation(i, instantiationContext);

			break;
		}
	}
}

void Compiler::mapGenericParams(shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext) {
	// DO NOT map functions because their generic parameters are saved in overloadings.
	if (node->getNodeType() == NodeType::Fn)
		return;

	if (instantiationContext.genericArgs->size() != node->genericParams.size())
		throw FatalCompilationError(
			Message(
				instantiationContext.genericArgs->at(0)->getLocation(),
				MessageType::Error,
				"Unmatched generic argument count"));

	for (size_t i = 0; i < node->genericParams.size(); ++i) {
		instantiationContext.mappedGenericArgs[node->genericParams[i]->name] = instantiationContext.genericArgs->at(i);
	}
}

shared_ptr<MemberNode> Compiler::instantiateGenericNode(shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext) {
	if (auto it = _genericCacheDir.find(node.get()); it != _genericCacheDir.end()) {
		if (auto subIt = it->second.find(*instantiationContext.genericArgs); subIt != it->second.end())
			return static_pointer_cast<MemberNode>(subIt->second->shared_from_this());
	}

	shared_ptr<MemberNode> newInstance = node->duplicate<MemberNode>();

	mapGenericParams(node, instantiationContext);
	walkNodeForGenericInstantiation(newInstance, instantiationContext);

	newInstance->genericArgs = *instantiationContext.genericArgs;
	newInstance->originalValue = node.get();

	_genericCacheDir[node.get()][*instantiationContext.genericArgs] = newInstance.get();

	return newInstance;
}

shared_ptr<FnOverloadingNode> Compiler::instantiateGenericFnOverloading(shared_ptr<FnOverloadingNode> overloading, GenericNodeInstantiationContext& instantiationContext) {
	mapGenericParams(overloading, instantiationContext);
	walkTypeNameNodeForGenericInstantiation(overloading->returnType, instantiationContext);

	auto newInstance = overloading->duplicate<FnOverloadingNode>();

	for (auto &j : newInstance->genericParams)
		walkNodeForGenericInstantiation(j, instantiationContext);

	for (auto &j : newInstance->params) {
		j.originalType = j.type;
		walkTypeNameNodeForGenericInstantiation(j.type, instantiationContext);
	}

	return newInstance;
}