#ifndef _SLKC_AST_MODULE_H_
#define _SLKC_AST_MODULE_H_

#include "expr.h"

namespace slkc {
	class AttributeNode;

	class MemberNode : public AstNode {
	public:
		MemberNode *parent = nullptr;  // DO NOT use WeakPtr because we want to set the parent during the copy constructor is executing.
		peff::String name;
		peff::DynArray<AstNodePtr<TypeNameNode>> genericArgs;
		slake::AccessModifier accessModifier = 0;
		peff::DynArray<AstNodePtr<AttributeNode>> attributes;

		SLKC_API MemberNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API MemberNode(const MemberNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~MemberNode();

		SLAKE_FORCEINLINE void setParent(MemberNode *parent) noexcept {
			this->parent = parent;
		}
	};

	class ImportNode;
	class VarDefStmtNode;

	class Parser;

	class ModuleNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::SharedPtr<Parser> parser;
		peff::DynArray<AstNodePtr<MemberNode>> members;
		peff::HashMap<std::string_view, size_t> memberIndices;
		peff::DynArray<AstNodePtr<ImportNode>> anonymousImports;
		peff::DynArray<AstNodePtr<VarDefStmtNode>> varDefStmts;

		bool isVarDefStmtsNormalized = false;

		SLKC_API ModuleNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, AstNodeType astNodeType = AstNodeType::Module);
		SLKC_API ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ModuleNode();

		[[nodiscard]] SLKC_API size_t pushMember(AstNodePtr<MemberNode> memberNode) noexcept;
		/// @brief Push and index a member.
		/// @param memberNode Member node to be added
		/// @return Whether the member is added successfully.
		[[nodiscard]] SLKC_API bool addMember(AstNodePtr<MemberNode> memberNode) noexcept;
		[[nodiscard]] SLKC_API bool indexMember(size_t indexInMemberArray) noexcept;
		/// @brief Remove a named member.
		/// @param name Name of the member to be removed.
		/// @return Whether the member is removed successfully.
		[[nodiscard]] SLKC_API bool removeMember(const std::string_view &name) noexcept;

		SLKC_API void setParser(peff::SharedPtr<Parser> parser);
	};
}

#endif
