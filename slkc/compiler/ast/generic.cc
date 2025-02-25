#include <slkc/compiler/compiler.h>

using namespace slake::slkc;

std::shared_ptr<AstNode> GenericParamNode::doDuplicate() {
	return std::make_shared<GenericParamNode>(*this);
}

std::shared_ptr<GenericParamNode> slake::slkc::lookupGenericParam(std::shared_ptr<AstNode> node, std::string name) {
	switch (node->getNodeType()) {
		case NodeType::Class: {
			std::shared_ptr<ClassNode> n = std::static_pointer_cast<ClassNode>(node);

			if (auto it = n->genericParamIndices.find(name); it != n->genericParamIndices.end())
				return n->genericParams[it->second];

			if (n->parent)
				return lookupGenericParam(n->parent->shared_from_this(), name);
			break;
		}
		case NodeType::Interface: {
			std::shared_ptr<InterfaceNode> n = std::static_pointer_cast<InterfaceNode>(node);

			if (auto it = n->genericParamIndices.find(name); it != n->genericParamIndices.end())
				return n->genericParams[it->second];

			if (n->parent)
				return lookupGenericParam(n->parent->shared_from_this(), name);
			break;
		}
		case NodeType::FnOverloadingValue: {
			std::shared_ptr<FnOverloadingNode> n = std::static_pointer_cast<FnOverloadingNode>(node);

			if (auto it = n->genericParamIndices.find(name); it != n->genericParamIndices.end())
				return n->genericParams[it->second];

			if (n->owner) {
				if (n->owner->parent)
					return lookupGenericParam(n->owner->parent->shared_from_this(), name);
			}
			break;
		}
		default:
			break;
	}

	return {};
}
