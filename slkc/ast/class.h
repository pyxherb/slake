#ifndef _SLKC_AST_CLASS_H_
#define _SLKC_AST_CLASS_H_

#include "var.h"
#include "fn.h"

namespace slkc {
	class ClassNode : public ModuleNode {
	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool is_cyclic_inheritance_checked = false;
		/// @brief Indicates if the class has cyclic inheritance.
		bool is_cyclic_inherited_flag = false;

		peff::DynArray<size_t> idx_generic_param_comma_tokens;
		size_t idx_langle_bracket_token = SIZE_MAX, idx_rangle_bracket_token = SIZE_MAX;

		bool is_generic_params_indexed = false;

		SLKC_API ClassNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ClassNode(const ClassNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ClassNode();

		SLKC_API peff::Option<CompilationError> is_cyclic_inherited(bool &whether_out);
		SLKC_API peff::Option<CompilationError> update_cyclic_inherited_status();
		SLAKE_FORCEINLINE void reset_cyclic_inheritance_flag() {
			is_cyclic_inheritance_checked = false;
			is_cyclic_inherited_flag = false;
		}

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class InterfaceNode : public ModuleNode {
	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool is_cyclic_inheritance_checked = false;
		/// @brief Indicates if the interface has cyclic inheritance.
		bool is_cyclic_inherited_flag = false;
		/// @brief Error indicates which type name caused the inheritance error.
		peff::Option<CompilationError> cyclic_inheritance_error;

		peff::DynArray<size_t> idx_generic_param_comma_tokens;
		size_t idx_langle_bracket_token = SIZE_MAX, idx_rangle_bracket_token = SIZE_MAX;

		bool is_generic_params_indexed = false;

		SLKC_API InterfaceNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API InterfaceNode(const InterfaceNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~InterfaceNode();

		SLKC_API peff::Option<CompilationError> is_cyclic_inherited(bool &whether_out);
		SLKC_API peff::Option<CompilationError> update_cyclic_inherited_status();
		SLAKE_FORCEINLINE void reset_cyclic_inheritance_flag() {
			is_cyclic_inheritance_checked = false;
			is_cyclic_inherited_flag = false;
			cyclic_inheritance_error.reset();
		}

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class StructNode : public ModuleNode {
	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool is_recursed_type_checked = false;
		/// @brief Indicates if the class has cyclic inheritance.
		bool is_recursed_type_flag = false;

		peff::DynArray<AstNodePtr<TypeNameNode>> impl_types;
		peff::DynArray<AstNodePtr<GenericParamNode>> generic_params;
		peff::HashMap<std::string_view, size_t> generic_param_indices;
		peff::DynArray<size_t> idx_generic_param_comma_tokens;
		size_t idx_langle_bracket_token = SIZE_MAX, idx_rangle_bracket_token = SIZE_MAX;

		bool is_generic_params_indexed = false;

		SLKC_API StructNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API StructNode(const StructNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~StructNode();

		SLKC_API peff::Option<CompilationError> is_recursed_type(bool &whether_out);
		SLKC_API peff::Option<CompilationError> update_recursed_type_status();
		SLAKE_FORCEINLINE void reset_recursed_type_flag() {
			is_recursed_type_checked = false;
			is_recursed_type_flag = false;
		}

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class EnumItemNode : public MemberNode {
	public:
		AstNodePtr<ExprNode> enum_value;
		AstNodePtr<ExprNode> filled_value;

		SLKC_API EnumItemNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API EnumItemNode(const EnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~EnumItemNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class ConstEnumNode : public ModuleNode {
	public:
		AstNodePtr<TypeNameNode> underlying_type;

		SLKC_API ConstEnumNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ConstEnumNode(const ConstEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ConstEnumNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class ScopedEnumNode : public ModuleNode {
	public:
		AstNodePtr<TypeNameNode> underlying_type;

		SLKC_API ScopedEnumNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ScopedEnumNode(const ScopedEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ScopedEnumNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class UnionEnumItemNode : public ModuleNode {
	public:
		SLKC_API UnionEnumItemNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnionEnumItemNode(const UnionEnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~UnionEnumItemNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class UnionEnumNode : public ModuleNode {
	public:
		/// @brief Indicates if the cyclic inheritance is already checked.
		bool is_recursed_type_checked = false;
		/// @brief Indicates if the class has cyclic inheritance.
		bool is_recursed_type_flag = false;

		peff::DynArray<size_t> idx_generic_param_comma_tokens;
		size_t idx_langle_bracket_token = SIZE_MAX, idx_rangle_bracket_token = SIZE_MAX;
		bool is_generic_params_indexed = false;

		SLKC_API UnionEnumNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnionEnumNode(const UnionEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~UnionEnumNode();

		SLKC_API peff::Option<CompilationError> is_recursed_type(bool &whether_out);
		SLKC_API peff::Option<CompilationError> update_recursed_type_status();
		SLAKE_FORCEINLINE void reset_recursed_type_flag() {
			is_recursed_type_checked = false;
			is_recursed_type_flag = false;
		}

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class ThisNode : public MemberNode {
	public:
		AstNodePtr<MemberNode> this_type;

		SLKC_API ThisNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ThisNode(const ThisNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ThisNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};
}

#endif
