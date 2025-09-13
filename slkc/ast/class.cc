#include "../comp/compiler.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> ClassNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<ClassNode> duplicatedNode(makeAstNode<ClassNode>(newAllocator, *this, newAllocator, succeeded));
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
	  implTypes(selfAllocator) {
}

SLKC_API ClassNode::ClassNode(const ClassNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ModuleNode(rhs, allocator, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator), implTypes(allocator) {
	if (!succeededOut) {
		return;
	}

	if (rhs.baseType && !(baseType = rhs.baseType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!implTypes.resize(rhs.implTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < implTypes.size(); ++i) {
		if (!(implTypes.at(i) = rhs.implTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
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

	memcpy(idxGenericParamCommaTokens.data(), rhs.idxGenericParamCommaTokens.data(), sizeof(size_t)* idxGenericParamCommaTokens.size());

	idxLAngleBracketToken = rhs.idxLAngleBracketToken;
	idxRAngleBracketToken = rhs.idxRAngleBracketToken;

	isGenericParamsIndexed = rhs.isGenericParamsIndexed;

	succeededOut = true;
}

SLKC_API ClassNode::~ClassNode() {
}

SLKC_API std::optional<CompilationError> ClassNode::isCyclicInherited(bool &whetherOut) {
	if (isCyclicInheritanceChecked) {
		whetherOut = isCyclicInheritedFlag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(updateCyclicInheritedFlag());

	whetherOut = isCyclicInheritedFlag;
	return {};
}

SLKC_API std::optional<CompilationError> ClassNode::updateCyclicInheritedFlag() {
	SLKC_RETURN_IF_COMP_ERROR(isBaseOf(document->sharedFromThis(), sharedFromThis().castTo<ClassNode>(), sharedFromThis().castTo<ClassNode>(), isCyclicInheritedFlag));

	isCyclicInheritanceChecked = true;
	return {};
}

SLKC_API AstNodePtr<AstNode> InterfaceNode::doDuplicate(peff::Alloc* newAllocator) const {
	bool succeeded = false;
	AstNodePtr<InterfaceNode> duplicatedNode(makeAstNode<InterfaceNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API InterfaceNode::InterfaceNode(
	peff::Alloc* selfAllocator,
	const peff::SharedPtr<Document>& document)
	: ModuleNode(selfAllocator, document, AstNodeType::Interface),
	genericParams(selfAllocator),
	genericParamIndices(selfAllocator),
	idxGenericParamCommaTokens(selfAllocator),
	implTypes(selfAllocator) {
}

SLKC_API InterfaceNode::InterfaceNode(const InterfaceNode& rhs, peff::Alloc* allocator, bool& succeededOut) : ModuleNode(rhs, allocator, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator), implTypes(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!implTypes.resize(rhs.implTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < implTypes.size(); ++i) {
		if (!(implTypes.at(i) = rhs.implTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
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

SLKC_API InterfaceNode::~InterfaceNode() {
}

SLKC_API std::optional<CompilationError> InterfaceNode::isCyclicInherited(bool &whetherOut) {
	if (isCyclicInheritanceChecked) {
		whetherOut = isCyclicInheritedFlag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(updateCyclicInheritedFlag());

	whetherOut = isCyclicInheritedFlag;
	return {};
}

SLKC_API std::optional<CompilationError> InterfaceNode::updateCyclicInheritedFlag() {
	peff::Set<AstNodePtr<InterfaceNode>> involvedInterfaces(document->allocator.get());

	if (auto e = collectInvolvedInterfaces(document->sharedFromThis(), sharedFromThis().castTo<InterfaceNode>(), involvedInterfaces, true); e) {
		if (e->errorKind == CompilationErrorKind::CyclicInheritedInterface) {
			isCyclicInheritedFlag = true;
			isCyclicInheritanceChecked = true;

			return {};
		}
		return e;
	}

	isCyclicInheritedFlag = false;
	isCyclicInheritanceChecked = true;

	return {};
}

SLKC_API AstNodePtr<AstNode> ThisNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<ThisNode> duplicatedNode(makeAstNode<ThisNode>(newAllocator, *this, newAllocator, succeeded));
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
