#include "typename.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> VoidTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<VoidTypeNameNode> duplicatedNode(makeAstNode<VoidTypeNameNode>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Void, selfAllocator, document) {
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(const VoidTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API VoidTypeNameNode::~VoidTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I8TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<I8TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I8TypeNameNode, AstNodeControlBlock<I8TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API I8TypeNameNode::I8TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I8, selfAllocator, document) {
}

SLKC_API I8TypeNameNode::I8TypeNameNode(const I8TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API I8TypeNameNode::~I8TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I16TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<I16TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I16TypeNameNode, AstNodeControlBlock<I16TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API I16TypeNameNode::I16TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I16, selfAllocator, document) {
}

SLKC_API I16TypeNameNode::I16TypeNameNode(const I16TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API I16TypeNameNode::~I16TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I32TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<I32TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API I32TypeNameNode::I32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I32, selfAllocator, document) {
}

SLKC_API I32TypeNameNode::I32TypeNameNode(const I32TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API I32TypeNameNode::~I32TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I64TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<I64TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I64TypeNameNode, AstNodeControlBlock<I64TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API I64TypeNameNode::I64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I64, selfAllocator, document) {
}

SLKC_API I64TypeNameNode::I64TypeNameNode(const I64TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API I64TypeNameNode::~I64TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U8TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<U8TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U8TypeNameNode, AstNodeControlBlock<U8TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API U8TypeNameNode::U8TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U8, selfAllocator, document) {
}

SLKC_API U8TypeNameNode::U8TypeNameNode(const U8TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API U8TypeNameNode::~U8TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U16TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<U16TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U16TypeNameNode, AstNodeControlBlock<U16TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API U16TypeNameNode::U16TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U16, selfAllocator, document) {
}

SLKC_API U16TypeNameNode::U16TypeNameNode(const U16TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API U16TypeNameNode::~U16TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U32TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<U32TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API U32TypeNameNode::U32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U32, selfAllocator, document) {
}

SLKC_API U32TypeNameNode::U32TypeNameNode(const U32TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API U32TypeNameNode::~U32TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U64TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<U64TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U64TypeNameNode, AstNodeControlBlock<U64TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API U64TypeNameNode::U64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U64, selfAllocator, document) {
}

SLKC_API U64TypeNameNode::U64TypeNameNode(const U64TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API U64TypeNameNode::~U64TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ISizeTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<ISizeTypeNameNode> duplicatedNode(makeAstNode<ISizeTypeNameNode>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::ISize, selfAllocator, document) {
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(const ISizeTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API ISizeTypeNameNode::~ISizeTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> USizeTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<USizeTypeNameNode> duplicatedNode(makeAstNode<USizeTypeNameNode>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::USize, selfAllocator, document) {
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(const USizeTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API USizeTypeNameNode::~USizeTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> F32TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<F32TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<F32TypeNameNode, AstNodeControlBlock<F32TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API F32TypeNameNode::F32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::F32, selfAllocator, document) {
}

SLKC_API F32TypeNameNode::F32TypeNameNode(const F32TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API F32TypeNameNode::~F32TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> F64TypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	peff::SharedPtr<F64TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<F64TypeNameNode, AstNodeControlBlock<F64TypeNameNode>>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API F64TypeNameNode::F64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::F64, selfAllocator, document) {
}

SLKC_API F64TypeNameNode::F64TypeNameNode(const F64TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API F64TypeNameNode::~F64TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> StringTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<StringTypeNameNode> duplicatedNode(makeAstNode<StringTypeNameNode>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API StringTypeNameNode::StringTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::String, selfAllocator, document) {
}

SLKC_API StringTypeNameNode::StringTypeNameNode(const StringTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API StringTypeNameNode::~StringTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> BoolTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<BoolTypeNameNode> duplicatedNode(makeAstNode<BoolTypeNameNode>(newAllocator, *this, newAllocator, context));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Bool, selfAllocator, document) {
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(const BoolTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API BoolTypeNameNode::~BoolTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ObjectTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<ObjectTypeNameNode> duplicatedNode(makeAstNode<ObjectTypeNameNode>(newAllocator, *this, newAllocator, context));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Object, selfAllocator, document) {
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(const ObjectTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API ObjectTypeNameNode::~ObjectTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> AnyTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	AstNodePtr<AnyTypeNameNode> duplicatedNode(makeAstNode<AnyTypeNameNode>(newAllocator, *this, newAllocator, context));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Any, selfAllocator, document) {
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(const AnyTypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : TypeNameNode(rhs, selfAllocator, context) {
}

SLKC_API AnyTypeNameNode::~AnyTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> CustomTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<CustomTypeNameNode> duplicatedNode(makeAstNode<CustomTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Custom, selfAllocator, document) {
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(const CustomTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context) {
	if(!(idRefPtr = duplicateIdRef(allocator, rhs.idRefPtr.get()))) {
		succeededOut = false;
		return;
	}

	contextNode = rhs.contextNode;

	succeededOut = true;
}

SLKC_API CustomTypeNameNode::~CustomTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> UnpackingTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnpackingTypeNameNode> duplicatedNode(makeAstNode<UnpackingTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API UnpackingTypeNameNode::UnpackingTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Unpacking, selfAllocator, document) {
}

SLKC_API UnpackingTypeNameNode::UnpackingTypeNameNode(const UnpackingTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context) {
	if (!(innerTypeName = rhs.innerTypeName->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API UnpackingTypeNameNode::~UnpackingTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ArrayTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ArrayTypeNameNode> duplicatedNode(makeAstNode<ArrayTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &elementType) : TypeNameNode(TypeNameKind::Array, selfAllocator, document), elementType(elementType) {
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(const ArrayTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context) {
	if(!(elementType = rhs.elementType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ArrayTypeNameNode::~ArrayTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> TupleTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<TupleTypeNameNode> duplicatedNode(makeAstNode<TupleTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API TupleTypeNameNode::TupleTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Tuple, selfAllocator, document), elementTypes(selfAllocator), idxCommaTokens(selfAllocator) {
}

SLKC_API TupleTypeNameNode::TupleTypeNameNode(const TupleTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context), elementTypes(allocator), idxCommaTokens(allocator) {
	if (!elementTypes.resize(rhs.elementTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < elementTypes.size(); ++i) {
		elementTypes.at(i) = rhs.elementTypes.at(i)->duplicate<TypeNameNode>(allocator);
	}

	if (!idxCommaTokens.resizeUninitialized(rhs.idxCommaTokens.size())) {
		succeededOut = false;
		return;
	}
	idxLBracketToken = rhs.idxLBracketToken;
	idxRBracketToken = rhs.idxRBracketToken;

	succeededOut = true;
}

SLKC_API TupleTypeNameNode::~TupleTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> SIMDTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<SIMDTypeNameNode> duplicatedNode(makeAstNode<SIMDTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API SIMDTypeNameNode::SIMDTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::SIMD, selfAllocator, document) {
}

SLKC_API SIMDTypeNameNode::SIMDTypeNameNode(const SIMDTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context) {
	if (!(elementType = rhs.elementType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(width = rhs.width->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	idxLAngleBracketToken = rhs.idxLAngleBracketToken;
	idxCommaToken = rhs.idxCommaToken;
	idxRAngleBracketToken = rhs.idxRAngleBracketToken;

	succeededOut = true;
}

SLKC_API SIMDTypeNameNode::~SIMDTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> FnTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<FnTypeNameNode> duplicatedNode(makeAstNode<FnTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API FnTypeNameNode::FnTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Fn, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API FnTypeNameNode::FnTypeNameNode(const FnTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context), paramTypes(allocator) {
	if (rhs.returnType && !(returnType = rhs.returnType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (rhs.thisType && !(thisType = rhs.thisType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!paramTypes.resize(rhs.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < paramTypes.size(); ++i) {
		if (!(paramTypes.at(i) = rhs.paramTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	hasVarArgs = rhs.hasVarArgs;
	isForAdl = rhs.isForAdl;

	succeededOut = true;
}

SLKC_API FnTypeNameNode::~FnTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> RefTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<RefTypeNameNode> duplicatedNode(makeAstNode<RefTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API RefTypeNameNode::RefTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referencedType) : TypeNameNode(TypeNameKind::Ref, selfAllocator, document), referencedType(referencedType) {
}

SLKC_API RefTypeNameNode::RefTypeNameNode(const RefTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context) {
	if(!(referencedType = rhs.referencedType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API RefTypeNameNode::~RefTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> TempRefTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<TempRefTypeNameNode> duplicatedNode(makeAstNode<TempRefTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API TempRefTypeNameNode::TempRefTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referencedType) : TypeNameNode(TypeNameKind::Ref, selfAllocator, document), referencedType(referencedType) {
}

SLKC_API TempRefTypeNameNode::TempRefTypeNameNode(const TempRefTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context) {
	if (!(referencedType = rhs.referencedType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API TempRefTypeNameNode::~TempRefTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ParamTypeListTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ParamTypeListTypeNameNode> duplicatedNode(makeAstNode<ParamTypeListTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API ParamTypeListTypeNameNode::ParamTypeListTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::ParamTypeList, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API ParamTypeListTypeNameNode::ParamTypeListTypeNameNode(const ParamTypeListTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context), paramTypes(allocator) {
	if (!paramTypes.resize(rhs.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < paramTypes.size(); ++i) {
		if (!(paramTypes.at(i) = rhs.paramTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	hasVarArgs = rhs.hasVarArgs;

	succeededOut = true;
}

SLKC_API ParamTypeListTypeNameNode::~ParamTypeListTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> UnpackedParamsTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnpackedParamsTypeNameNode> duplicatedNode(makeAstNode<UnpackedParamsTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API UnpackedParamsTypeNameNode::UnpackedParamsTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::UnpackedParams, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API UnpackedParamsTypeNameNode::UnpackedParamsTypeNameNode(const UnpackedParamsTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context), paramTypes(allocator) {
	if (!paramTypes.resize(rhs.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < paramTypes.size(); ++i) {
		if (!(paramTypes.at(i) = rhs.paramTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	hasVarArgs = rhs.hasVarArgs;

	succeededOut = true;
}

SLKC_API UnpackedParamsTypeNameNode::~UnpackedParamsTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> UnpackedArgsTypeNameNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnpackedArgsTypeNameNode> duplicatedNode(makeAstNode<UnpackedArgsTypeNameNode>(newAllocator, *this, newAllocator, context, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.template castTo<AstNode>();
}

SLKC_API UnpackedArgsTypeNameNode::UnpackedArgsTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::UnpackedArgs, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API UnpackedArgsTypeNameNode::UnpackedArgsTypeNameNode(const UnpackedArgsTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut) : TypeNameNode(rhs, allocator, context), paramTypes(allocator) {
	if (!paramTypes.resize(rhs.paramTypes.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < paramTypes.size(); ++i) {
		if (!(paramTypes.at(i) = rhs.paramTypes.at(i)->duplicate<TypeNameNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	hasVarArgs = rhs.hasVarArgs;

	succeededOut = true;
}

SLKC_API UnpackedArgsTypeNameNode::~UnpackedArgsTypeNameNode() {
}
