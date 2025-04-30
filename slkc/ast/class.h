#ifndef _SLKC_AST_CLASS_H_
#define _SLKC_AST_CLASS_H_

#include "var.h"
#include "fn.h"

namespace slkc {
	class ClassNode : public ModuleNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<TypeNameNode> baseType;
		peff::DynArray<peff::SharedPtr<TypeNameNode>> implementedTypes;
		peff::DynArray<peff::SharedPtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;

		bool isGenericParamsIndexed = false;

		SLKC_API ClassNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ClassNode(const ClassNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ClassNode();
	};

	class InterfaceNode : public ModuleNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<peff::SharedPtr<TypeNameNode>> implementedTypes;
		peff::DynArray<peff::SharedPtr<GenericParamNode>> genericParams;
		peff::HashMap<std::string_view, size_t> genericParamIndices;
		peff::DynArray<size_t> idxGenericParamCommaTokens;
		size_t idxLAngleBracketToken = SIZE_MAX, idxRAngleBracketToken = SIZE_MAX;

		bool isGenericParamsIndexed = false;

		SLKC_API InterfaceNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API InterfaceNode(const InterfaceNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~InterfaceNode();
	};

	class ThisNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<MemberNode> thisType;

		SLKC_API ThisNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ThisNode(const ThisNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ThisNode();
	};
}

#endif
