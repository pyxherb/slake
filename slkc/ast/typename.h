#ifndef _SLKC_AST_TYPENAME_H_
#define _SLKC_AST_TYPENAME_H_

#include "expr.h"

namespace slkc {
	class VoidTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API VoidTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API VoidTypeNameNode(const VoidTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~VoidTypeNameNode();
	};

	class I8TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API I8TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API I8TypeNameNode(const I8TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~I8TypeNameNode();
	};

	class I16TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API I16TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API I16TypeNameNode(const I16TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~I16TypeNameNode();
	};

	class I32TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API I32TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API I32TypeNameNode(const I32TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~I32TypeNameNode();
	};

	class I64TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API I64TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API I64TypeNameNode(const I64TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~I64TypeNameNode();
	};

	class U8TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API U8TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API U8TypeNameNode(const U8TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~U8TypeNameNode();
	};

	class U16TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API U16TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API U16TypeNameNode(const U16TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~U16TypeNameNode();
	};

	class U32TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API U32TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API U32TypeNameNode(const U32TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~U32TypeNameNode();
	};

	class U64TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API U64TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API U64TypeNameNode(const U64TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~U64TypeNameNode();
	};

	class ISizeTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API ISizeTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ISizeTypeNameNode(const ISizeTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~ISizeTypeNameNode();
	};

	class USizeTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API USizeTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API USizeTypeNameNode(const USizeTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~USizeTypeNameNode();
	};

	class F32TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API F32TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API F32TypeNameNode(const F32TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~F32TypeNameNode();
	};

	class F64TypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API F64TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API F64TypeNameNode(const F64TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~F64TypeNameNode();
	};

	class StringTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API StringTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API StringTypeNameNode(const StringTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~StringTypeNameNode();
	};

	class BoolTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API BoolTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API BoolTypeNameNode(const BoolTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~BoolTypeNameNode();
	};

	class ObjectTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API ObjectTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ObjectTypeNameNode(const ObjectTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~ObjectTypeNameNode();
	};

	class AnyTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API AnyTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AnyTypeNameNode(const AnyTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~AnyTypeNameNode();
	};

	class MemberNode;

	class CustomTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		IdRefPtr id_ref_ptr;
		peff::WeakPtr<MemberNode> context_node;
		peff::WeakPtr<MemberNode> cached_resolve_result;

		SLKC_API CustomTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API CustomTypeNameNode(const CustomTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~CustomTypeNameNode();
	};

	class UnpackingTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> inner_type_name;

		SLKC_API UnpackingTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnpackingTypeNameNode(const UnpackingTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~UnpackingTypeNameNode();
	};

	class ArrayTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> element_type;

		SLKC_API ArrayTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &element_type);
		SLKC_API ArrayTypeNameNode(const ArrayTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ArrayTypeNameNode();
	};

	class FnTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> return_type;
		AstNodePtr<TypeNameNode> this_type;
		peff::DynArray<AstNodePtr<TypeNameNode>> param_types;
		bool has_var_args = false;
		bool is_for_adl = false;

		SLKC_API FnTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API FnTypeNameNode(const FnTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~FnTypeNameNode();
	};

	class RefTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> referenced_type;

		SLKC_API RefTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referenced_type);
		SLKC_API RefTypeNameNode(const RefTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~RefTypeNameNode();
	};

	class TempRefTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> referenced_type;

		SLKC_API TempRefTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referenced_type);
		SLKC_API TempRefTypeNameNode(const TempRefTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~TempRefTypeNameNode();
	};

	class TupleTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> element_types;

		size_t idx_lbracket_token = SIZE_MAX,
			   idx_rbracket_token = SIZE_MAX;

		peff::DynArray<size_t> idx_comma_tokens;

		SLKC_API TupleTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API TupleTypeNameNode(const TupleTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~TupleTypeNameNode();
	};

	class SIMDTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> element_type;
		AstNodePtr<ExprNode> width;

		size_t idx_langle_bracket_token = SIZE_MAX,
			   idx_comma_token = SIZE_MAX,
			   idx_rangle_bracket_token = SIZE_MAX;

		SLKC_API SIMDTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API SIMDTypeNameNode(const SIMDTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~SIMDTypeNameNode();
	};

	class ParamTypeListTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> param_types;
		bool has_var_args = false;

		SLKC_API ParamTypeListTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ParamTypeListTypeNameNode(const ParamTypeListTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ParamTypeListTypeNameNode();
	};

	class UnpackedParamsTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> param_types;
		bool has_var_args = false;

		SLKC_API UnpackedParamsTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnpackedParamsTypeNameNode(const UnpackedParamsTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~UnpackedParamsTypeNameNode();
	};

	class UnpackedArgsTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<TypeNameNode>> param_types;
		bool has_var_args = false;

		SLKC_API UnpackedArgsTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnpackedArgsTypeNameNode(const UnpackedArgsTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~UnpackedArgsTypeNameNode();
	};

	class NullTypeNameNode : public TypeNameNode {
	public:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API NullTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API NullTypeNameNode(const NullTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~NullTypeNameNode();
	};

	enum class MetaTypeKind : uint8_t {
		Id = 0,
		Tokens,
		Expr,
		Stmt,
		Type,
		Class,
		Struct,
		Interface,
		GenericParam
	};
}

#endif
