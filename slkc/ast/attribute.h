#ifndef _SLKC_AST_ATTRIBUTE_H_
#define _SLKC_AST_ATTRIBUTE_H_

#include "var.h"
#include "fn.h"

namespace slkc {
	class AttributeDefNode : public ModuleNode {
	public:
		peff::DynArray<AstNodePtr<GenericParamNode>> generic_params;
		peff::HashMap<std::string_view, size_t> generic_param_indices;
		peff::DynArray<size_t> idx_generic_param_comma_tokens;
		size_t idx_langle_bracket_token = SIZE_MAX, idx_rangle_bracket_token = SIZE_MAX;

		bool is_generic_params_indexed = false;

		SLKC_API AttributeDefNode(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeDefNode(const AttributeDefNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~AttributeDefNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};

	class AttributeNode : public AstNode {
	public:
		IdRefPtr attribute_name;
		peff::DynArray<AstNodePtr<ExprNode>> field_data;
		peff::DynArray<size_t> idx_comma_tokens;

		AstNodePtr<TypeNameNode> applied_for;

		SLKC_API AttributeNode(peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AttributeNode(const AttributeNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~AttributeNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};
}

#endif
