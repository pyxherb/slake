#include "generic.h"

using namespace slkc;

SLKC_API GenericConstraint::GenericConstraint(peff::Alloc *selfAllocator) : selfAllocator(selfAllocator), implTypes(selfAllocator) {}
SLKC_API GenericConstraint::~GenericConstraint() {}
SLKC_API void GenericConstraint::dealloc() noexcept {
	peff::destroyAndRelease<GenericConstraint>(selfAllocator.get(), this, alignof(GenericConstraint));
}

GenericConstraintPtr slkc::duplicateGenericConstraint(peff::Alloc *allocator, const GenericConstraint *constraint){
	GenericConstraintPtr ptr(peff::allocAndConstruct<GenericConstraint>(allocator, alignof(GenericConstraint), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (constraint->baseType && !(ptr->baseType = constraint->baseType->duplicate<TypeNameNode>(allocator))) {
		return nullptr;
	}

	if (!ptr->implTypes.resize(constraint->implTypes.size())) {
		return nullptr;
	}

	for (size_t i = 0; i < ptr->implTypes.size(); ++i) {
		if (!(ptr->implTypes.at(i) = constraint->implTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			return nullptr;
		}
	}

	return ptr;
}

SLKC_API ParamTypeListGenericConstraint::ParamTypeListGenericConstraint(peff::Alloc *selfAllocator) : selfAllocator(selfAllocator), argTypes(selfAllocator) {}
SLKC_API ParamTypeListGenericConstraint::~ParamTypeListGenericConstraint() {}
SLKC_API void ParamTypeListGenericConstraint::dealloc() noexcept {
	peff::destroyAndRelease<ParamTypeListGenericConstraint>(selfAllocator.get(), this, alignof(ParamTypeListGenericConstraint));
}

ParamTypeListGenericConstraintPtr slkc::duplicateParamTypeListGenericConstraint(peff::Alloc *allocator, const ParamTypeListGenericConstraint *constraint) {
	ParamTypeListGenericConstraintPtr ptr(peff::allocAndConstruct<ParamTypeListGenericConstraint>(allocator, alignof(ParamTypeListGenericConstraint), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (!ptr->argTypes.resize(constraint->argTypes.size())) {
		return nullptr;
	}

	for (size_t i = 0; i < ptr->argTypes.size(); ++i) {
		if (!(ptr->argTypes.at(i) = constraint->argTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			return nullptr;
		}
	}

	ptr->hasVarArg = constraint->hasVarArg;

	return ptr;
}

SLKC_API AstNodePtr<AstNode> GenericParamNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<GenericParamNode> duplicatedNode(makeAstNode<GenericParamNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API GenericParamNode::GenericParamNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::GenericParam, selfAllocator, document) {
}

SLKC_API GenericParamNode::GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : MemberNode(rhs, allocator, context, succeededOut) {
	if (!succeededOut) {
		return;
	}

	if (rhs.genericConstraint && !(genericConstraint = duplicateGenericConstraint(allocator, rhs.genericConstraint.get()))) {
		succeededOut = true;
		return;
	}

	if (rhs.paramTypeListGenericConstraint && !(paramTypeListGenericConstraint = duplicateParamTypeListGenericConstraint(allocator, rhs.paramTypeListGenericConstraint.get()))) {
		succeededOut = true;
		return;
	}

	isParamTypeList = rhs.isParamTypeList;

	succeededOut = true;
}

SLKC_API GenericParamNode::~GenericParamNode() {
}
