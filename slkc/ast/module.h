#ifndef _SLKC_AST_MODULE_H_
#define _SLKC_AST_MODULE_H_

#include "expr.h"

namespace slkc {
	class AttributeNode;
	class MemberNode;
	class ImportNode;
	class GenericParamNode;

	class Scope final {
	private:

	public:
		const peff::RcObjectPtr<peff::Alloc> allocator;

		peff::DynArray<AstNodePtr<MemberNode>> _members;
		peff::HashMap<std::string_view, size_t> _member_indices;

		///
		/// @brief The owner member node, which connects to the outer scope node.
		///
		MemberNode *owner = nullptr;

		peff::DynArray<AstNodePtr<ImportNode>> anonymous_imports;

		AstNodePtr<TypeNameNode> base_type;
		peff::DynArray<AstNodePtr<TypeNameNode>> impl_types;

		peff::DynArray<AstNodePtr<GenericParamNode>> generic_params;
		peff::HashMap<std::string_view, size_t> generic_param_indices;

		SLKC_API Scope(peff::Alloc *allocator) noexcept;
		SLKC_API ~Scope();

		SLAKE_FORCEINLINE static Scope *alloc(peff::Alloc *allocator) noexcept {
			return peff::alloc_and_construct<Scope>(allocator, alignof(Scope), allocator);
		}

		SLAKE_FORCEINLINE void dealloc() noexcept {
			peff::destroy_and_release<Scope>(allocator.get(), this, alignof(Scope));
		}

		SLKC_API Scope *duplicate(peff::Alloc *allocator, DuplicationContext &context);

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
		SLAKE_FORCEINLINE AstNodePtr<MemberNode> get_member(const std::string_view &name) const noexcept {
			return _members.at(_member_indices.at(name));
		}
		SLAKE_FORCEINLINE AstNodePtr<MemberNode> get_member(size_t index) const noexcept {
			return _members.at(index);
		}
		SLAKE_FORCEINLINE AstNodePtr<MemberNode> try_get_member(const std::string_view &name) const noexcept {
			if (auto it = _member_indices.find(name); it != _member_indices.end()) {
				return _members.at(it.value());
			}
			return {};
		}
		SLAKE_FORCEINLINE size_t get_member_num() const noexcept {
			return _members.size();
		}
		SLAKE_FORCEINLINE size_t get_indexed_member_num() const noexcept {
			return _member_indices.size();
		}
		SLAKE_FORCEINLINE decltype(_members)::Iterator members_begin() noexcept {
			return _members.begin();
		}
		SLAKE_FORCEINLINE decltype(_members)::Iterator members_end() noexcept {
			return _members.end();
		}
		SLAKE_FORCEINLINE decltype(_members)::ConstIterator members_begin_const() const noexcept {
			return _members.begin_const();
		}
		SLAKE_FORCEINLINE decltype(_members)::ConstIterator members_end_const() const noexcept {
			return _members.end_const();
		}
		SLAKE_FORCEINLINE const decltype(_member_indices) &get_members_indices() const noexcept {
			return _member_indices;
		}
		SLAKE_FORCEINLINE const AstNodePtr<MemberNode> *get_members_data() const noexcept {
			return _members.data();
		}
		SLAKE_FORCEINLINE decltype(_members) &get_members() noexcept {
			return _members;
		}
		SLAKE_FORCEINLINE const decltype(_members) &get_members_const() const noexcept {
			return _members;
		}

		[[nodiscard]] SLKC_API size_t push_generic_param(AstNodePtr<GenericParamNode> generic_param_node) noexcept;
		[[nodiscard]] SLKC_API bool add_generic_param(AstNodePtr<GenericParamNode> generic_param_node) noexcept;
		[[nodiscard]] SLKC_API bool index_generic_param(size_t index_in_member_array) noexcept;
		SLKC_API void remove_generic_param(const std::string_view &name) noexcept;
		SLAKE_FORCEINLINE AstNodePtr<GenericParamNode> get_generic_param(const std::string_view &name) const noexcept {
			return generic_params.at(generic_param_indices.at(name));
		}
		SLAKE_FORCEINLINE AstNodePtr<GenericParamNode> get_generic_param(size_t index) const noexcept {
			return generic_params.at(index);
		}
		SLAKE_FORCEINLINE AstNodePtr<GenericParamNode> try_get_generic_param(const std::string_view &name) const noexcept {
			if (auto it = generic_param_indices.find(name); it != generic_param_indices.end()) {
				return generic_params.at(it.value());
			}
			return {};
		}
		SLAKE_FORCEINLINE size_t get_generic_param_num() const noexcept {
			return generic_params.size();
		}
		SLAKE_FORCEINLINE size_t get_indexed_generic_param_num() const noexcept {
			return generic_param_indices.size();
		}
		SLAKE_FORCEINLINE decltype(generic_params)::Iterator generic_params_begin() noexcept {
			return generic_params.begin();
		}
		SLAKE_FORCEINLINE decltype(generic_params)::Iterator generic_params_end() noexcept {
			return generic_params.end();
		}
		SLAKE_FORCEINLINE decltype(generic_params)::ConstIterator generic_params_begin_const() const noexcept {
			return generic_params.begin_const();
		}
		SLAKE_FORCEINLINE decltype(generic_params)::ConstIterator generic_params_end_const() const noexcept {
			return generic_params.end_const();
		}
		SLAKE_FORCEINLINE const AstNodePtr<GenericParamNode> *get_generic_params_data() const noexcept {
			return generic_params.data();
		}
		SLAKE_FORCEINLINE decltype(generic_params) &get_generic_params() noexcept {
			return generic_params;
		}
		SLAKE_FORCEINLINE const decltype(generic_params) &get_generic_params_const() const noexcept {
			return generic_params;
		}
	};

	class MemberNode : public AstNode {
	protected:
		friend class Scope;

	public:
		MemberNode *outer = nullptr;  // DO NOT use WeakPtr because we want to set the parent during the copy constructor is executing.
		peff::String name;
		peff::DynArray<AstNodePtr<TypeNameNode>> generic_args;
		peff::DynArray<AstNodePtr<AttributeNode>> attributes;
		peff::UniquePtr<Scope, peff::DeallocableDeleter<Scope>> scope;
		slake::AccessModifier access_modifier = 0;

		SLKC_API MemberNode(AstNodeType ast_node_type, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API MemberNode(const MemberNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~MemberNode();

		SLAKE_FORCEINLINE Scope *get_scope() const noexcept {
			return scope.get();
		}

		SLAKE_FORCEINLINE Scope *alloc_scope() noexcept {
			if(!(scope = Scope::alloc(self_allocator.get())))
				return nullptr;
			scope->owner = this;
			return scope.get();
		}

		SLAKE_FORCEINLINE void set_parent(MemberNode *parent) noexcept {
			this->outer = parent;
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

	class VarDefStmtNode;

	class Parser;

	class FriendDeclNode : public AstNode {
	public:
	};

	class ModuleNode : public MemberNode {
	public:
		peff::SharedPtr<Parser> parser;
		peff::DynArray<AstNodePtr<VarDefStmtNode>> var_def_stmts;

		SLKC_API ModuleNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, AstNodeType ast_node_type = AstNodeType::Module);
		SLKC_API ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ModuleNode();

		SLKC_API void set_parser(const peff::SharedPtr<Parser> &parser);

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};
}

#endif
