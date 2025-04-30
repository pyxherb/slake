#include "class.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> ClassNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ClassNode> duplicatedNode(peff::makeShared<ClassNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ClassNode::ClassNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(selfAllocator, document, AstNodeType::Class),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator),
	  implementedTypes(selfAllocator) {
}

SLKC_API ClassNode::ClassNode(const ClassNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ModuleNode(rhs, allocator, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator), implementedTypes(allocator) {
	if (!succeededOut) {
		return;
	}

	if (rhs.baseType && !(baseType = rhs.baseType->duplicate<TypeNameNode>(allocator))) {
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

	if (!genericParams.resize(rhs.genericParams.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericParams.size(); ++i) {
		if (!(genericParams.at(i) = rhs.genericParams.at(i)->duplicate<GenericParamNode>(allocator))) {
			succeededOut = false;
			return;
		}

		genericParams.at(i)->setParent(this);
	}

	for (const auto& [k, v] : rhs.genericParamIndices) {
		if (!genericParamIndices.insert(genericParams.at(v)->name, +v)) {
			succeededOut = false;
			return;
		}
	}

	if (!idxGenericParamCommaTokens.resize(rhs.idxGenericParamCommaTokens.size())) {
		succeededOut = false;
		return;
	}

	memcpy(idxGenericParamCommaTokens.data(), rhs.idxGenericParamCommaTokens.data(), sizeof(size_t) * idxGenericParamCommaTokens.size());

	idxLAngleBracketToken = rhs.idxLAngleBracketToken;
	idxRAngleBracketToken = rhs.idxRAngleBracketToken;

	isGenericParamsIndexed = rhs.isGenericParamsIndexed;

	succeededOut = true;
}

SLKC_API ClassNode::~ClassNode() {
}

SLKC_API peff::SharedPtr<AstNode> InterfaceNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<InterfaceNode> duplicatedNode(peff::makeShared<InterfaceNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API InterfaceNode::InterfaceNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(selfAllocator, document, AstNodeType::Interface),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator),
	  implementedTypes(selfAllocator) {
}

SLKC_API InterfaceNode::InterfaceNode(const InterfaceNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ModuleNode(rhs, allocator, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator), implementedTypes(allocator) {
	if (!succeededOut) {
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

	if (!genericParams.resize(rhs.genericParams.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericParams.size(); ++i) {
		if (!(genericParams.at(i) = rhs.genericParams.at(i)->duplicate<GenericParamNode>(allocator))) {
			succeededOut = false;
			return;
		}

		genericParams.at(i)->setParent(this);
	}

	for (const auto &[k, v] : rhs.genericParamIndices) {
		if (!genericParamIndices.insert(genericParams.at(v)->name, +v)) {
			succeededOut = false;
			return;
		}
	}

	if (!idxGenericParamCommaTokens.resize(rhs.idxGenericParamCommaTokens.size())) {
		succeededOut = false;
		return;
	}

	memcpy(idxGenericParamCommaTokens.data(), rhs.idxGenericParamCommaTokens.data(), sizeof(size_t) * idxGenericParamCommaTokens.size());

	idxLAngleBracketToken = rhs.idxLAngleBracketToken;
	idxRAngleBracketToken = rhs.idxRAngleBracketToken;

	isGenericParamsIndexed = rhs.isGenericParamsIndexed;

	succeededOut = true;
}

SLKC_API InterfaceNode::~InterfaceNode() {
}

SLKC_API peff::SharedPtr<AstNode> ThisNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ThisNode> duplicatedNode(peff::makeShared<ThisNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ThisNode::ThisNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::This, selfAllocator, document) {
}

SLKC_API ThisNode::ThisNode(const ThisNode &rhs, peff::Alloc *allocator, bool &succeededOut) : MemberNode(rhs, allocator, succeededOut) {
	if (!succeededOut) {
		return;
	}

	thisType = rhs.thisType;

	succeededOut = true;
}

SLKC_API ThisNode::~ThisNode() {
}
