#include "typename.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> VoidTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<VoidTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<VoidTypeNameNode, AstNodeControlBlock<VoidTypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Void, selfAllocator, document) {
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(const VoidTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API VoidTypeNameNode::~VoidTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> I8TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I8TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I8TypeNameNode, AstNodeControlBlock<I8TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I8TypeNameNode::I8TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I8, selfAllocator, document) {
}

SLKC_API I8TypeNameNode::I8TypeNameNode(const I8TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API I8TypeNameNode::~I8TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> I16TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I16TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I16TypeNameNode, AstNodeControlBlock<I16TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I16TypeNameNode::I16TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I16, selfAllocator, document) {
}

SLKC_API I16TypeNameNode::I16TypeNameNode(const I16TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API I16TypeNameNode::~I16TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> I32TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I32TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I32TypeNameNode::I32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I32, selfAllocator, document) {
}

SLKC_API I32TypeNameNode::I32TypeNameNode(const I32TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API I32TypeNameNode::~I32TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> I64TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I64TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<I64TypeNameNode, AstNodeControlBlock<I64TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I64TypeNameNode::I64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I64, selfAllocator, document) {
}

SLKC_API I64TypeNameNode::I64TypeNameNode(const I64TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API I64TypeNameNode::~I64TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> U8TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U8TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U8TypeNameNode, AstNodeControlBlock<U8TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U8TypeNameNode::U8TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U8, selfAllocator, document) {
}

SLKC_API U8TypeNameNode::U8TypeNameNode(const U8TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API U8TypeNameNode::~U8TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> U16TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U16TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U16TypeNameNode, AstNodeControlBlock<U16TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U16TypeNameNode::U16TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U16, selfAllocator, document) {
}

SLKC_API U16TypeNameNode::U16TypeNameNode(const U16TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API U16TypeNameNode::~U16TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> U32TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U32TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U32TypeNameNode::U32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U32, selfAllocator, document) {
}

SLKC_API U32TypeNameNode::U32TypeNameNode(const U32TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API U32TypeNameNode::~U32TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> U64TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U64TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<U64TypeNameNode, AstNodeControlBlock<U64TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U64TypeNameNode::U64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U64, selfAllocator, document) {
}

SLKC_API U64TypeNameNode::U64TypeNameNode(const U64TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API U64TypeNameNode::~U64TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> ISizeTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<ISizeTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<ISizeTypeNameNode, AstNodeControlBlock<ISizeTypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::ISize, selfAllocator, document) {
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(const ISizeTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API ISizeTypeNameNode::~ISizeTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> USizeTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<USizeTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<USizeTypeNameNode, AstNodeControlBlock<USizeTypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::USize, selfAllocator, document) {
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(const USizeTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API USizeTypeNameNode::~USizeTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> F32TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<F32TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<F32TypeNameNode, AstNodeControlBlock<F32TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API F32TypeNameNode::F32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::F32, selfAllocator, document) {
}

SLKC_API F32TypeNameNode::F32TypeNameNode(const F32TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API F32TypeNameNode::~F32TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> F64TypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<F64TypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<F64TypeNameNode, AstNodeControlBlock<F64TypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API F64TypeNameNode::F64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::F64, selfAllocator, document) {
}

SLKC_API F64TypeNameNode::F64TypeNameNode(const F64TypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API F64TypeNameNode::~F64TypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> StringTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<StringTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<StringTypeNameNode, AstNodeControlBlock<StringTypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API StringTypeNameNode::StringTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::String, selfAllocator, document) {
}

SLKC_API StringTypeNameNode::StringTypeNameNode(const StringTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API StringTypeNameNode::~StringTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> BoolTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<BoolTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<BoolTypeNameNode, AstNodeControlBlock<BoolTypeNameNode>>(newAllocator, *this, newAllocator));
	if(!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Bool, selfAllocator, document) {
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(const BoolTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API BoolTypeNameNode::~BoolTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> ObjectTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<ObjectTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<ObjectTypeNameNode, AstNodeControlBlock<ObjectTypeNameNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Object, selfAllocator, document) {
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(const ObjectTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API ObjectTypeNameNode::~ObjectTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> AnyTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<AnyTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<AnyTypeNameNode, AstNodeControlBlock<AnyTypeNameNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Any, selfAllocator, document) {
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(const AnyTypeNameNode &rhs, peff::Alloc *selfAllocator) : TypeNameNode(rhs, selfAllocator) {
}

SLKC_API AnyTypeNameNode::~AnyTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> CustomTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<CustomTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<CustomTypeNameNode, AstNodeControlBlock<CustomTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Custom, selfAllocator, document) {
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(const CustomTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator) {
	if(!(idRefPtr = duplicateIdRef(allocator, rhs.idRefPtr.get()))) {
		succeededOut = false;
		return;
	}

	contextNode = rhs.contextNode;

	succeededOut = true;
}

SLKC_API CustomTypeNameNode::~CustomTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> UnpackingTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<UnpackingTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<UnpackingTypeNameNode, AstNodeControlBlock<UnpackingTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API UnpackingTypeNameNode::UnpackingTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Unpacking, selfAllocator, document) {
}

SLKC_API UnpackingTypeNameNode::UnpackingTypeNameNode(const UnpackingTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator) {
	if (!(innerTypeName = rhs.innerTypeName->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API UnpackingTypeNameNode::~UnpackingTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> ArrayTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ArrayTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<ArrayTypeNameNode, AstNodeControlBlock<ArrayTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<TypeNameNode> &elementType) : TypeNameNode(TypeNameKind::Array, selfAllocator, document), elementType(elementType) {
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(const ArrayTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator) {
	if(!(elementType = rhs.elementType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API ArrayTypeNameNode::~ArrayTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> FnTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<FnTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<FnTypeNameNode, AstNodeControlBlock<FnTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API FnTypeNameNode::FnTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Fn, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API FnTypeNameNode::FnTypeNameNode(const FnTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator), paramTypes(allocator) {
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

SLKC_API peff::SharedPtr<AstNode> RefTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<RefTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<RefTypeNameNode, AstNodeControlBlock<RefTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API RefTypeNameNode::RefTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<TypeNameNode> &referencedType) : TypeNameNode(TypeNameKind::Ref, selfAllocator, document), referencedType(referencedType) {
}

SLKC_API RefTypeNameNode::RefTypeNameNode(const RefTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator) {
	if(!(referencedType = rhs.referencedType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API RefTypeNameNode::~RefTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> TempRefTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<TempRefTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<TempRefTypeNameNode, AstNodeControlBlock<TempRefTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API TempRefTypeNameNode::TempRefTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<TypeNameNode> &referencedType) : TypeNameNode(TypeNameKind::Ref, selfAllocator, document), referencedType(referencedType) {
}

SLKC_API TempRefTypeNameNode::TempRefTypeNameNode(const TempRefTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator) {
	if (!(referencedType = rhs.referencedType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API TempRefTypeNameNode::~TempRefTypeNameNode() {
}

SLKC_API peff::SharedPtr<AstNode> ParamTypeListTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ParamTypeListTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<ParamTypeListTypeNameNode, AstNodeControlBlock<ParamTypeListTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API ParamTypeListTypeNameNode::ParamTypeListTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::ParamTypeList, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API ParamTypeListTypeNameNode::ParamTypeListTypeNameNode(const ParamTypeListTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator), paramTypes(allocator) {
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

SLKC_API peff::SharedPtr<AstNode> UnpackedParamsTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<UnpackedParamsTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<UnpackedParamsTypeNameNode, AstNodeControlBlock<UnpackedParamsTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API UnpackedParamsTypeNameNode::UnpackedParamsTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::UnpackedParams, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API UnpackedParamsTypeNameNode::UnpackedParamsTypeNameNode(const UnpackedParamsTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator), paramTypes(allocator) {
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

SLKC_API peff::SharedPtr<AstNode> UnpackedArgsTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<UnpackedArgsTypeNameNode> duplicatedNode(peff::makeSharedWithControlBlock<UnpackedArgsTypeNameNode, AstNodeControlBlock<UnpackedArgsTypeNameNode>>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API UnpackedArgsTypeNameNode::UnpackedArgsTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::UnpackedArgs, selfAllocator, document), paramTypes(selfAllocator) {
}

SLKC_API UnpackedArgsTypeNameNode::UnpackedArgsTypeNameNode(const UnpackedArgsTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut) : TypeNameNode(rhs, allocator), paramTypes(allocator) {
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
