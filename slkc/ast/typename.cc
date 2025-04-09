#include "typename.h"

using namespace slkc;

SLKC_API peff::SharedPtr<AstNode> VoidTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<VoidTypeNameNode> duplicatedNode(peff::makeShared<VoidTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<I8TypeNameNode> duplicatedNode(peff::makeShared<I8TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<I16TypeNameNode> duplicatedNode(peff::makeShared<I16TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<I32TypeNameNode> duplicatedNode(peff::makeShared<I32TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<I64TypeNameNode> duplicatedNode(peff::makeShared<I64TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<U8TypeNameNode> duplicatedNode(peff::makeShared<U8TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<U16TypeNameNode> duplicatedNode(peff::makeShared<U16TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<U32TypeNameNode> duplicatedNode(peff::makeShared<U32TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<U64TypeNameNode> duplicatedNode(peff::makeShared<U64TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<ISizeTypeNameNode> duplicatedNode(peff::makeShared<ISizeTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<USizeTypeNameNode> duplicatedNode(peff::makeShared<USizeTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<F32TypeNameNode> duplicatedNode(peff::makeShared<F32TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<F64TypeNameNode> duplicatedNode(peff::makeShared<F64TypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<StringTypeNameNode> duplicatedNode(peff::makeShared<StringTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<BoolTypeNameNode> duplicatedNode(peff::makeShared<BoolTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<ObjectTypeNameNode> duplicatedNode(peff::makeShared<ObjectTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<AnyTypeNameNode> duplicatedNode(peff::makeShared<AnyTypeNameNode>(newAllocator, *this, newAllocator));
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
	peff::SharedPtr<CustomTypeNameNode> duplicatedNode(peff::makeShared<CustomTypeNameNode>(newAllocator, *this, newAllocator, succeeded));
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

SLKC_API peff::SharedPtr<AstNode> ArrayTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<ArrayTypeNameNode> duplicatedNode(peff::makeShared<ArrayTypeNameNode>(newAllocator, *this, newAllocator, succeeded));
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

SLKC_API peff::SharedPtr<AstNode> RefTypeNameNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	peff::SharedPtr<RefTypeNameNode> duplicatedNode(peff::makeShared<RefTypeNameNode>(newAllocator, *this, newAllocator, succeeded));
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
