#ifndef _SLKC_AST_MODULE_H_
#define _SLKC_AST_MODULE_H_

#include "expr.h"

namespace slkc {
	class AttributeNode;

	class MemberNode : public AstNode {
	public:
		MemberNode *parent = nullptr;  // DO NOT use WeakPtr because we want to set the parent during the copy constructor is executing.
		peff::String name;
		peff::DynArray<AstNodePtr<TypeNameNode>> generic_args;
		slake::AccessModifier access_modifier = 0;
		peff::DynArray<AstNodePtr<AttributeNode>> attributes;

		SLKC_API MemberNode(AstNodeType ast_node_type, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API MemberNode(const MemberNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~MemberNode();

		SLAKE_FORCEINLINE void set_parent(MemberNode *parent) noexcept {
			this->parent = parent;
		}

		SLAKE_FORCEINLINE bool is_public() const noexcept {
			return access_modifier & slake::ACCESS_PUBLIC;
		}

		SLAKE_FORCEINLINE bool is_static() const noexcept {
			return access_modifier & slake::ACCESS_STATIC;
		}

		SLAKE_FORCEINLINE bool is_native() const noexcept {
			return access_modifier & slake::ACCESS_NATIVE;
		}
	};

	class ImportNode;
	class VarDefStmtNode;

	class Parser;

	class FriendDeclNode : public AstNode {
	public:

	};

	class ModuleNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::SharedPtr<Parser> parser;
		peff::DynArray<AstNodePtr<MemberNode>> members;
		peff::HashMap<std::string_view, size_t> member_indices;
		peff::DynArray<AstNodePtr<ImportNode>> anonymous_imports;
		peff::DynArray<AstNodePtr<VarDefStmtNode>> var_def_stmts;

		bool is_var_def_stmts_normalized = false;

		SLKC_API ModuleNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, AstNodeType ast_node_type = AstNodeType::Module);
		SLKC_API ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ModuleNode();

		[[nodiscard]] SLKC_API size_t push_member(AstNodePtr<MemberNode> member_node) noexcept;
		/// @brief Push and index a member.
		/// @param member_node Member node to be added
		/// @return Whether the member is added successfully.
		[[nodiscard]] SLKC_API bool add_member(AstNodePtr<MemberNode> member_node) noexcept;
		[[nodiscard]] SLKC_API bool index_member(size_t index_in_member_array) noexcept;
		/// @brief Remove a named member.
		/// @param name Name of the member to be removed.
		/// @return Whether the member is removed successfully.
		SLKC_API void remove_member(const std::string_view &name) noexcept;

		SLKC_API void set_parser(const peff::SharedPtr<Parser> &parser);
	};
}

#endif
