#include "fn.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> FnNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<FnNode> duplicatedNode(makeAstNode<FnNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API FnNode::FnNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Fn, selfAllocator, document),
	  overloadings(selfAllocator) {
}

SLKC_API FnNode::FnNode(const FnNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : MemberNode(rhs, allocator, context, succeededOut), overloadings(allocator) {
	if (!succeededOut) {
		return;
	}

	if (!overloadings.resize(rhs.overloadings.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < overloadings.size(); ++i) {
		if (!(overloadings.at(i) = rhs.overloadings.at(i)->duplicate<FnOverloadingNode>(allocator))) {
			succeededOut = false;
			return;
		}

		overloadings.at(i)->setParent(this);
	}

	succeededOut = true;
}

SLKC_API FnNode::~FnNode() {
}

SLKC_API AstNodePtr<AstNode> FnOverloadingNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<FnOverloadingNode> duplicatedNode(makeAstNode<FnOverloadingNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API FnOverloadingNode::FnOverloadingNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::FnOverloading, selfAllocator, document),
	  params(selfAllocator),
	  paramIndices(selfAllocator),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxParamCommaTokens(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator) {
}

SLKC_API FnOverloadingNode::FnOverloadingNode(const FnOverloadingNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut)
	: MemberNode(rhs, allocator, context, succeededOut),
	  params(allocator),
	  paramIndices(allocator),
	  genericParams(allocator),
	  genericParamIndices(allocator),
	  idxParamCommaTokens(allocator),
	  idxGenericParamCommaTokens(allocator),
	  lAngleBracketIndex(rhs.lAngleBracketIndex),
	  rAngleBracketIndex(rhs.rAngleBracketIndex),
	  lvalueMarkerIndex(rhs.lvalueMarkerIndex),
	  returnTypeTokenIndex(rhs.returnTypeTokenIndex),
	  overloadingKind(rhs.overloadingKind),
	  fnFlags(rhs.fnFlags) {
	/* if (rhs.body && !(body = rhs.body->duplicate<CodeBlockStmtNode>(allocator))) {
		succeededOut = false;
		return;
	}*/

	if (!context.pushTask([this, &rhs, allocator, &context]() -> bool {
			if (rhs.returnType && !(returnType = rhs.returnType->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeededOut = false;
		return;
	}

	if (!context.pushTask([this, &rhs, allocator, &context]() -> bool {
			if (rhs.overridenType && !(overridenType = rhs.overridenType->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeededOut = false;
		return;
	}

	if (!params.resize(rhs.params.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < params.size(); ++i) {
		if (!context.pushTask([this, i, &rhs, allocator, &context]() -> bool {
				if (!(params.at(i) = rhs.params.at(i)->duplicate<VarNode>(allocator))) {
					return false;
				}

				if (!(paramIndices.insert(params.at(i)->name, +i))) {
					return false;
				}

				params.at(i)->setParent(this);
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

				if (!genericParamIndices.insert(genericParams.at(i)->name, +i))
					return false;

				genericParams.at(i)->setParent(this);
				return true;
			})) {
			succeededOut = false;
			return;
		}
	}

	isParamsIndexed = rhs.isParamsIndexed;

	if (!idxGenericParamCommaTokens.resize(rhs.idxGenericParamCommaTokens.size())) {
		succeededOut = false;
		return;
	}
	memcpy(idxGenericParamCommaTokens.data(), rhs.idxGenericParamCommaTokens.data(), idxGenericParamCommaTokens.size() * sizeof(size_t));

	succeededOut = true;
}

SLKC_API FnOverloadingNode::~FnOverloadingNode() {
}
