#ifndef _SLKC_AST_MODULE_H_
#define _SLKC_AST_MODULE_H_

#include "expr.h"
#include "stmt.h"

namespace slkc {
	class MemberNode : public AstNode {
	public:
		peff::WeakPtr<AstNode> parent;
		peff::String name;

		SLKC_API MemberNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API MemberNode(const MemberNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~MemberNode();

		SLAKE_FORCEINLINE void setParent(peff::WeakPtr<AstNode> &&parent) noexcept {
			this->parent = std::move(parent);
		}
	};

	class ModuleNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::HashMap<std::string_view, peff::SharedPtr<MemberNode>> members;
		peff::DynArray<peff::SharedPtr<VarDefStmtNode>> varDefStmts;

		SLKC_API ModuleNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, AstNodeType astNodeType = AstNodeType::Module);
		SLKC_API ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ModuleNode();

		[[nodiscard]] SLKC_API bool addMember(MemberNode *memberNode) noexcept;
		SLKC_API void removeMember(const std::string_view &name) noexcept;
	};
}

#endif
