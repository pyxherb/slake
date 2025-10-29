#include "../comp/compiler.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> ClassNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ClassNode> duplicatedNode(makeAstNode<ClassNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
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

SLKC_API ClassNode::ClassNode(const ClassNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : ModuleNode(rhs, allocator, context, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator), implTypes(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!context.pushTask([this, &rhs, allocator, &context]() -> bool {
			if (rhs.baseType && !(baseType = rhs.baseType->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeededOut = false;
		return;
	}

	if (!implTypes.resize(rhs.implTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < implTypes.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(implTypes.at(i) = rhs.implTypes.at(i)->duplicate<TypeNameNode>(allocator)))
					return false;
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	if (!genericParams.resize(rhs.genericParams.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericParams.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(genericParams.at(i) = rhs.genericParams.at(i)->duplicate<GenericParamNode>(allocator)))
					return false;

				genericParams.at(i)->setParent(this);
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	for (const auto &[k, v] : rhs.genericParamIndices) {
		if (!context.pushTask([this, v, &rhs, allocator, &context]() -> bool {
				if (!genericParamIndices.insert(genericParams.at(v)->name, +v)) {
					return false;
				}
				return true;
			})) {
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

SLKC_API peff::Option<CompilationError> ClassNode::isCyclicInherited(bool &whetherOut) {
	if (isCyclicInheritanceChecked) {
		whetherOut = isCyclicInheritedFlag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(updateCyclicInheritedStatus());

	whetherOut = isCyclicInheritedFlag;
	return {};
}

SLKC_API peff::Option<CompilationError> ClassNode::updateCyclicInheritedStatus() {
	SLKC_RETURN_IF_COMP_ERROR(isBaseOf(document->sharedFromThis(), sharedFromThis().template castTo<ClassNode>(), sharedFromThis().template castTo<ClassNode>(), isCyclicInheritedFlag));

	isCyclicInheritanceChecked = true;
	return {};
}

SLKC_API AstNodePtr<AstNode> InterfaceNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<InterfaceNode> duplicatedNode(makeAstNode<InterfaceNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API InterfaceNode::InterfaceNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(selfAllocator, document, AstNodeType::Interface),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator),
	  implTypes(selfAllocator) {
}

SLKC_API InterfaceNode::InterfaceNode(const InterfaceNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : ModuleNode(rhs, allocator, context, succeededOut), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator), implTypes(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!implTypes.resize(rhs.implTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < implTypes.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(implTypes.at(i) = rhs.implTypes.at(i)->duplicate<TypeNameNode>(allocator)))
					return false;
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	if (!genericParams.resize(rhs.genericParams.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericParams.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(genericParams.at(i) = rhs.genericParams.at(i)->duplicate<GenericParamNode>(allocator)))
					return false;

				genericParams.at(i)->setParent(this);
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	for (const auto &[k, v] : rhs.genericParamIndices) {
		if (!context.pushTask([this, v, &rhs, allocator, &context]() -> bool {
				if (!genericParamIndices.insert(genericParams.at(v)->name, +v)) {
					return false;
				}
				return true;
			})) {
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

SLKC_API peff::Option<CompilationError> InterfaceNode::isCyclicInherited(bool &whetherOut) {
	if (isCyclicInheritanceChecked) {
		whetherOut = isCyclicInheritedFlag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(updateCyclicInheritedStatus());

	whetherOut = isCyclicInheritedFlag;
	return {};
}

SLKC_API peff::Option<CompilationError> InterfaceNode::updateCyclicInheritedStatus() {
	peff::Set<AstNodePtr<InterfaceNode>> involvedInterfaces(document->allocator.get());

	if (auto e = collectInvolvedInterfaces(document->sharedFromThis(), sharedFromThis().template castTo<InterfaceNode>(), involvedInterfaces, true); e) {
		if (e->errorKind == CompilationErrorKind::CyclicInheritedInterface) {
			isCyclicInheritedFlag = true;
			isCyclicInheritanceChecked = true;
			if (!cyclicInheritanceError.hasValue()) {
				cyclicInheritanceError = std::move(*e);
			}
			e.reset();

			return {};
		}
		return e;
	}

	isCyclicInheritedFlag = false;
	isCyclicInheritanceChecked = true;

	return {};
}

SLKC_API AstNodePtr<AstNode> StructNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<StructNode> duplicatedNode(makeAstNode<StructNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API StructNode::StructNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(selfAllocator, document, AstNodeType::Struct),
	  implTypes(selfAllocator),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator) {
}

SLKC_API StructNode::StructNode(const StructNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : ModuleNode(rhs, allocator, context, succeededOut), implTypes(allocator), genericParams(allocator), genericParamIndices(allocator), idxGenericParamCommaTokens(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!implTypes.resize(rhs.implTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < implTypes.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(implTypes.at(i) = rhs.implTypes.at(i)->duplicate<TypeNameNode>(allocator)))
					return false;
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	if (!genericParams.resize(rhs.genericParams.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < genericParams.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(genericParams.at(i) = rhs.genericParams.at(i)->duplicate<GenericParamNode>(allocator)))
					return false;

				genericParams.at(i)->setParent(this);
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	for (const auto &[k, v] : rhs.genericParamIndices) {
		if (!context.pushTask([this, v, &rhs, allocator, &context]() -> bool {
				if (!genericParamIndices.insert(genericParams.at(v)->name, +v)) {
					return false;
				}
				return true;
			})) {
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

SLKC_API StructNode::~StructNode() {
}

SLKC_API peff::Option<CompilationError> StructNode::isRecursedType(bool &whetherOut) {
	if (isRecursedTypeChecked) {
		whetherOut = isRecursedTypeFlag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(updateRecursedTypeStatus());

	whetherOut = isRecursedTypeFlag;
	return {};
}

SLKC_API peff::Option<CompilationError> StructNode::updateRecursedTypeStatus() {
	isRecursedTypeFlag = false;

	SLKC_RETURN_IF_COMP_ERROR(isStructRecursed(document->sharedFromThis(), sharedFromThis().template castTo<StructNode>()));

	isRecursedTypeFlag = true;

	isRecursedTypeChecked = true;
	return {};
}

SLKC_API AstNodePtr<AstNode> ThisNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ThisNode> duplicatedNode(makeAstNode<ThisNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API ThisNode::ThisNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::This, selfAllocator, document) {
}

SLKC_API ThisNode::ThisNode(const ThisNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : MemberNode(rhs, allocator, context, succeededOut) {
	if (!succeededOut) {
		return;
	}

	thisType = rhs.thisType;

	succeededOut = true;
}

SLKC_API ThisNode::~ThisNode() {
}

struct CollectInvolvedInterfacesFrame {
	AstNodePtr<InterfaceNode> interfaceNode;
	size_t index;
};

struct CollectInvolvedInterfacesContext {
	peff::List<CollectInvolvedInterfacesFrame> frames;

	SLAKE_FORCEINLINE CollectInvolvedInterfacesContext(peff::Alloc *allocator) : frames(allocator) {}
};

static peff::Option<CompilationError> _collectInvolvedInterfaces(
	peff::SharedPtr<Document> document,
	CollectInvolvedInterfacesContext &context,
	AstNodePtr<InterfaceNode> interfaceNode,
	peff::Set<AstNodePtr<InterfaceNode>> &walkedInterfaces) {
	if (!context.frames.pushBack({ interfaceNode, 0 }))
		return genOutOfMemoryCompError();

	while (context.frames.size()) {
		CollectInvolvedInterfacesFrame &curFrame = context.frames.back();

		const AstNodePtr<InterfaceNode> &curInterface = curFrame.interfaceNode;

		// Check if the interface has cyclic inheritance.
		if (!curFrame.index) {
			for (auto &i : context.frames) {
				if ((&i != &curFrame) && (i.interfaceNode == curFrame.interfaceNode)) {
					auto source = context.frames.front();
					return CompilationError(source.interfaceNode->implTypes.at(source.index - 1)->tokenRange, CompilationErrorKind::CyclicInheritedInterface);
				}
			}
		}
		if (curFrame.index >= curInterface->implTypes.size()) {
			if (!walkedInterfaces.insert(AstNodePtr<InterfaceNode>(curInterface)))
				return genOutOfMemoryCompError();
			context.frames.popBack();
			continue;
		}

		AstNodePtr<TypeNameNode> t = curInterface->implTypes.at(curFrame.index);

		AstNodePtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, t.template castTo<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->getAstNodeType() != AstNodeType::Interface) {
			goto malformed;
		}

		if (!context.frames.pushBack({ m.template castTo<InterfaceNode>(), 0 }))
			return genOutOfMemoryCompError();

		++curFrame.index;
	}

	return {};

malformed:
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::collectInvolvedInterfaces(
	peff::SharedPtr<Document> document,
	const AstNodePtr<InterfaceNode> &derived,
	peff::Set<AstNodePtr<InterfaceNode>> &walkedInterfaces,
	bool insertSelf) {
	if (walkedInterfaces.contains(derived)) {
		return {};
	}
	if (insertSelf) {
		if (!walkedInterfaces.insert(AstNodePtr<InterfaceNode>(derived))) {
			return genOutOfMemoryCompError();
		}
	}

	CollectInvolvedInterfacesContext context(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(_collectInvolvedInterfaces(document, context, derived, walkedInterfaces));

	return {};

malformed:
	return {};
}

struct StructRecursionCheckFrame {
	AstNodePtr<StructNode> structNode;
	size_t index;
};

struct StructRecursionCheckContext {
	peff::List<StructRecursionCheckFrame> frames;

	SLAKE_FORCEINLINE StructRecursionCheckContext(peff::Alloc *allocator) : frames(allocator) {}
};

static peff::Option<CompilationError> _isStructRecursed(
	peff::SharedPtr<Document> document,
	StructRecursionCheckContext &context,
	AstNodePtr<StructNode> structNode,
	peff::Set<AstNodePtr<StructNode>> &walkedStructs) {
	if (!context.frames.pushBack({ structNode, 0 }))
		return genOutOfMemoryCompError();

	while (context.frames.size()) {
		StructRecursionCheckFrame &curFrame = context.frames.back();

		const AstNodePtr<StructNode> &curStruct = curFrame.structNode;

		if (!curFrame.index) {
			for (auto &i : context.frames) {
				if ((&i != &curFrame) && (i.structNode == curFrame.structNode)) {
					auto source = context.frames.front();
					return CompilationError(source.structNode->members.at(source.index - 1).template castTo<VarNode>()->type->tokenRange, CompilationErrorKind::RecursedStruct);
				}
			}
		}
		if (curFrame.index >= curStruct->members.size()) {
			if (!walkedStructs.insert(AstNodePtr<StructNode>(curStruct)))
				return genOutOfMemoryCompError();
			context.frames.popBack();
			continue;
		}

		AstNodePtr<MemberNode> v = curStruct->members.at(curFrame.index);

		if (v->getAstNodeType() == AstNodeType::Var) {
			AstNodePtr<VarNode> varMember = v.template castTo<VarNode>();

			AstNodePtr<MemberNode> m;

			if (auto t = varMember->type; varMember->type->typeNameKind == TypeNameKind::Custom) {
				SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, t.template castTo<CustomTypeNameNode>(), m));

				if (m->getAstNodeType() == AstNodeType::Struct)
					if (!context.frames.pushBack({ m.template castTo<StructNode>(), 0 }))
						return genOutOfMemoryCompError();
			}
		}

		++curFrame.index;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isStructRecursed(
	peff::SharedPtr<Document> document,
	const AstNodePtr<StructNode> &derived) {
	StructRecursionCheckContext context(document->allocator.get());
	peff::Set<AstNodePtr<StructNode>> walkedStructs(document->allocator.get());

	return _isStructRecursed(document, context, derived, walkedStructs);
}

SLKC_API peff::Option<CompilationError> slkc::isImplementedByInterface(
	peff::SharedPtr<Document> document,
	const AstNodePtr<InterfaceNode> &base,
	const AstNodePtr<InterfaceNode> &derived,
	bool &whetherOut) {
	peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(document, derived, interfaces, true));

	whetherOut = interfaces.contains(base);
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isImplementedByClass(
	peff::SharedPtr<Document> document,
	const AstNodePtr<InterfaceNode> &base,
	const AstNodePtr<ClassNode> &derived,
	bool &whetherOut) {
	peff::Set<AstNodePtr<ClassNode>> walkedClasses(document->allocator.get());

	if (!walkedClasses.insert(AstNodePtr<ClassNode>(derived))) {
		return genOutOfMemoryCompError();
	}

	AstNodePtr<ClassNode> currentClass = derived;
	AstNodePtr<TypeNameNode> currentType = derived->baseType;

	while (currentType) {
		if (currentType->typeNameKind != TypeNameKind::Custom) {
			goto malformed;
		}

		AstNodePtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, currentType.template castTo<CustomTypeNameNode>(), m));

		if (m->getAstNodeType() != AstNodeType::Class) {
			goto malformed;
		}

		currentClass = m.template castTo<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walkedClasses.contains(currentClass)) {
			whetherOut = true;
			return {};
		}

		for (size_t i = 0; i < currentClass->implTypes.size(); ++i) {
			AstNodePtr<TypeNameNode> t = derived->implTypes.at(i);

			AstNodePtr<MemberNode> m;
			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, t.template castTo<CustomTypeNameNode>(), m));

			if (!m) {
				goto malformed;
			}

			if (m->getAstNodeType() != AstNodeType::Interface) {
				goto malformed;
			}

			AstNodePtr<InterfaceNode> interfaceNode = m.template castTo<InterfaceNode>();

			if (interfaceNode == base) {
				whetherOut = true;
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(document, base, interfaceNode, whetherOut));

			if (whetherOut) {
				whetherOut = true;
				return {};
			}
		}

		if (!walkedClasses.insert(AstNodePtr<ClassNode>(currentClass))) {
			return genOutOfMemoryCompError();
		}
	}

	whetherOut = false;
	return {};

malformed:
	whetherOut = false;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::isBaseOf(
	peff::SharedPtr<Document> document,
	const AstNodePtr<ClassNode> &base,
	const AstNodePtr<ClassNode> &derived,
	bool &whetherOut) {
	peff::Set<AstNodePtr<ClassNode>> walkedClasses(document->allocator.get());

	if (!walkedClasses.insert(AstNodePtr<ClassNode>(derived))) {
		return genOutOfMemoryCompError();
	}

	AstNodePtr<ClassNode> currentClass = derived;
	AstNodePtr<TypeNameNode> currentType;

	while ((currentType = currentClass->baseType)) {
		if (currentType->typeNameKind != TypeNameKind::Custom) {
			goto malformed;
		}

		AstNodePtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(document, currentType.template castTo<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->getAstNodeType() != AstNodeType::Class) {
			goto malformed;
		}

		currentClass = m.template castTo<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walkedClasses.contains(currentClass)) {
			whetherOut = true;
			return {};
		}

		if (currentClass == base) {
			whetherOut = true;
			return {};
		}

		if (!walkedClasses.insert(AstNodePtr<ClassNode>(currentClass))) {
			return genOutOfMemoryCompError();
		}
	}

	whetherOut = false;
	return {};

malformed:
	whetherOut = false;
	return {};
}
