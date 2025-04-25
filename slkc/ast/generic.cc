#include "generic.h"

using namespace slkc;

SLKC_API GenericConstraint::GenericConstraint(peff::Alloc *selfAllocator) : selfAllocator(selfAllocator), implementedTypes(selfAllocator) {}
SLKC_API GenericConstraint::~GenericConstraint() {}
SLKC_API void GenericConstraint::dealloc() noexcept {
	peff::destroyAndRelease<GenericConstraint>(selfAllocator.get(), this, alignof(GenericConstraint));
}

GenericConstraintPtr slkc::duplicateGenericConstraint(peff::Alloc *allocator, const GenericConstraint *constraint){
	GenericConstraintPtr ptr(peff::allocAndConstruct<GenericConstraint>(allocator, alignof(GenericConstraint), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (!(ptr->baseType = constraint->baseType->duplicate<TypeNameNode>(allocator))) {
		return nullptr;
	}

	if (!ptr->implementedTypes.resize(constraint->implementedTypes.size())) {
		return nullptr;
	}

	for (size_t i = 0; i < ptr->implementedTypes.size(); ++i) {
		if (!(ptr->implementedTypes.at(i) = constraint->implementedTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			return nullptr;
		}
	}

	return ptr;
}

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
	: MemberNode(AstNodeType::GenericParam, selfAllocator, document) {
}

SLKC_API GenericParamNode::GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut) {
	if (!succeededOut) {
		return;
	}

	if (genericConstraint && !(genericConstraint = duplicateGenericConstraint(allocator, rhs.genericConstraint.get()))) {
		succeededOut = true;
		return;
	}

	succeededOut = true;
}

SLKC_API GenericParamNode::~GenericParamNode() {
}
