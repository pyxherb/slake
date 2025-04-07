#include "generic.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> GenericParamNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<GenericParamNode> duplicatedNode(peff::makeShared<GenericParamNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API GenericParamNode::GenericParamNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::GenericParam, selfAllocator, document),
	  implementedTypes(selfAllocator) {
}

SLKC_API GenericParamNode::GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut), implementedTypes(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!(baseType = rhs.baseType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!implementedTypes.resize(rhs.implementedTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < implementedTypes.size(); ++i) {
		if (!(implementedTypes.at(i) = rhs.implementedTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}

SLKC_API GenericParamNode::~GenericParamNode() {
}
