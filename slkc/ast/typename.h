#ifndef _SLKC_AST_TYPENAME_H_
#define _SLKC_AST_TYPENAME_H_

#include "expr.h"

namespace slkc {
	class VoidTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API VoidTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API VoidTypeNameNode(const VoidTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~VoidTypeNameNode();
	};

	class I8TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API I8TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API I8TypeNameNode(const I8TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~I8TypeNameNode();
	};

	class I16TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API I16TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API I16TypeNameNode(const I16TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~I16TypeNameNode();
	};

	class I32TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API I32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API I32TypeNameNode(const I32TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~I32TypeNameNode();
	};

	class I64TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API I64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API I64TypeNameNode(const I64TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~I64TypeNameNode();
	};

	class U8TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API U8TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API U8TypeNameNode(const U8TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~U8TypeNameNode();
	};

	class U16TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API U16TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API U16TypeNameNode(const U16TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~U16TypeNameNode();
	};

	class U32TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API U32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API U32TypeNameNode(const U32TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~U32TypeNameNode();
	};

	class U64TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API U64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API U64TypeNameNode(const U64TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~U64TypeNameNode();
	};

	class ISizeTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API ISizeTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ISizeTypeNameNode(const ISizeTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~ISizeTypeNameNode();
	};

	class USizeTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API USizeTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API USizeTypeNameNode(const USizeTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~USizeTypeNameNode();
	};

	class F32TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API F32TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API F32TypeNameNode(const F32TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~F32TypeNameNode();
	};

	class F64TypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API F64TypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API F64TypeNameNode(const F64TypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~F64TypeNameNode();
	};

	class StringTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API StringTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API StringTypeNameNode(const StringTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~StringTypeNameNode();
	};

	class BoolTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API BoolTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API BoolTypeNameNode(const BoolTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~BoolTypeNameNode();
	};

	class ObjectTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API ObjectTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ObjectTypeNameNode(const ObjectTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~ObjectTypeNameNode();
	};

	class AnyTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API AnyTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API AnyTypeNameNode(const AnyTypeNameNode &rhs, peff::Alloc *selfAllocator);
		SLKC_API virtual ~AnyTypeNameNode();
	};

	class MemberNode;

	class CustomTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		IdRefPtr idRefPtr;
		peff::WeakPtr<MemberNode> contextNode;
		peff::WeakPtr<MemberNode> cachedResolveResult;

		SLKC_API CustomTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API CustomTypeNameNode(const CustomTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~CustomTypeNameNode();
	};

	class UnpackingTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		AstNodePtr<TypeNameNode> innerTypeName;

		SLKC_API UnpackingTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnpackingTypeNameNode(const UnpackingTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~UnpackingTypeNameNode();
	};

	class ArrayTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		AstNodePtr<TypeNameNode> elementType;

		SLKC_API ArrayTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &elementType);
		SLKC_API ArrayTypeNameNode(const ArrayTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ArrayTypeNameNode();
	};

	class FnTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		AstNodePtr<TypeNameNode> returnType;
		AstNodePtr<TypeNameNode> thisType;
		peff::DynArray<AstNodePtr<TypeNameNode>> paramTypes;
		bool hasVarArgs = false;
		bool isForAdl = false;

		SLKC_API FnTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnTypeNameNode(const FnTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~FnTypeNameNode();
	};

	class RefTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		AstNodePtr<TypeNameNode> referencedType;

		SLKC_API RefTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referencedType);
		SLKC_API RefTypeNameNode(const RefTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~RefTypeNameNode();
	};

	class TempRefTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		AstNodePtr<TypeNameNode> referencedType;

		SLKC_API TempRefTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referencedType);
		SLKC_API TempRefTypeNameNode(const TempRefTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~TempRefTypeNameNode();
	};

	class TupleTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> elementTypes;

		size_t idxLBracketToken = SIZE_MAX,
			   idxRBracketToken = SIZE_MAX;

		peff::DynArray<size_t> idxCommaTokens;

		SLKC_API TupleTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API TupleTypeNameNode(const TupleTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~TupleTypeNameNode();
	};

	class SIMDTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		AstNodePtr<TypeNameNode> elementType;
		AstNodePtr<ExprNode> width;

		size_t idxLAngleBracketToken = SIZE_MAX,
			   idxCommaToken = SIZE_MAX,
			   idxRAngleBracketToken = SIZE_MAX;

		SLKC_API SIMDTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API SIMDTypeNameNode(const SIMDTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~SIMDTypeNameNode();
	};

	class ParamTypeListTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> paramTypes;
		bool hasVarArgs = false;

		SLKC_API ParamTypeListTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ParamTypeListTypeNameNode(const ParamTypeListTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ParamTypeListTypeNameNode();
	};

	class UnpackedParamsTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> paramTypes;
		bool hasVarArgs = false;

		SLKC_API UnpackedParamsTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnpackedParamsTypeNameNode(const UnpackedParamsTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~UnpackedParamsTypeNameNode();
	};

	class UnpackedArgsTypeNameNode : public TypeNameNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> paramTypes;
		bool hasVarArgs = false;

		SLKC_API UnpackedArgsTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnpackedArgsTypeNameNode(const UnpackedArgsTypeNameNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~UnpackedArgsTypeNameNode();
	};
}

#endif
