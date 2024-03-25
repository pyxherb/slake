#include <slkc/compiler/compiler.h>

using namespace slake::slkc;

shared_ptr<AstNode> GenericParamNode::doDuplicate() {
	return make_shared<GenericParamNode>(*this);
}

shared_ptr<GenericParamNode> slake::slkc::lookupGenericParam(shared_ptr<AstNode> node, string name) {
	switch (node->getNodeType()) {
		case NodeType::Class: {
			shared_ptr<ClassNode> n = static_pointer_cast<ClassNode>(node);

			if (auto it = n->genericParamIndices.find(name); it != n->genericParamIndices.end())
				return n->genericParams[it->second];

			if (n->parent)
				return lookupGenericParam(n->parent->shared_from_this(), name);
			break;
		}
		case NodeType::Interface: {
			shared_ptr<InterfaceNode> n = static_pointer_cast<InterfaceNode>(node);

			if (auto it = n->genericParamIndices.find(name); it != n->genericParamIndices.end())
				return n->genericParams[it->second];

			if (n->parent)
				return lookupGenericParam(n->parent->shared_from_this(), name);
			break;
		}
		case NodeType::Trait: {
			shared_ptr<TraitNode> n = static_pointer_cast<TraitNode>(node);

			if (auto it = n->genericParamIndices.find(name); it != n->genericParamIndices.end())
				return n->genericParams[it->second];

			if (n->parent)
				return lookupGenericParam(n->parent->shared_from_this(), name);
			break;
		}
		default:
			break;
	}

	return {};
}
