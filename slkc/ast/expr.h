#ifndef _SLKC_AST_EXPR_H_
#define _SLKC_AST_EXPR_H_

#include "typename_base.h"
#include "idref.h"
#include <peff/containers/hashmap.h>

namespace slkc {
	enum class ExprKind {
		Unary,	  // Unary operation
		Binary,	  // Binary operation
		Ternary,  // Ternary operation
		IdRef,	  // Identifier reference

		HeadedIdRef,  // Headed identifier reference

		I8,		 // i8 literal
		I16,	 // i16 literal
		I32,	 // i32 literal
		I64,	 // i64 literal
		U8,		 // u8 literal
		U16,	 // u16 literal
		U32,	 // u32 literal
		U64,	 // u64 literal
		F32,	 // f32 literal
		F64,	 // f64 literal
		String,	 // String literal
		Bool,	 // bool literal
		Null,	 // null

		InitializerList,  // Initializer list

		Call,  // Call

		New,  // New

		Alloca,	 // Alloca

		Cast,  // Cast

		Match,	// Match expression

		Wrapper,  // Expression wrapper

		RegIndex,  // Register reference

		TypeName,  // Type name

		BCLabel,  // Byte code label

		Bad,  // Bad expression
	};

	enum class UnaryOp {
		LNot,
		Not,
		Neg,
		Unpacking
	};

	class ExprNode : public AstNode {
	public:
		ExprKind expr_kind;

		SLKC_API ExprNode(ExprKind expr_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ExprNode(const ExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~ExprNode();
	};

	class UnaryExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		UnaryOp unary_op;
		AstNodePtr<ExprNode> operand;

		SLKC_API UnaryExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnaryExprNode(const UnaryExprNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~UnaryExprNode();
	};

	enum class BinaryOp {
		Add = 0,
		Sub,
		Mul,
		Div,
		Mod,
		And,
		Or,
		Xor,
		LAnd,
		LOr,
		Shl,
		Shr,

		Assign,
		AddAssign,
		SubAssign,
		MulAssign,
		DivAssign,
		ModAssign,
		AndAssign,
		OrAssign,
		XorAssign,
		ShlAssign,
		ShrAssign,

		Eq,
		Neq,
		StrictEq,
		StrictNeq,
		Lt,
		Gt,
		LtEq,
		GtEq,
		Cmp,
		Subscript,

		Comma
	};

	SLKC_API const char *get_binary_operator_overloading_name(BinaryOp op);

	class BinaryExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		BinaryOp binary_op;
		AstNodePtr<ExprNode> lhs, rhs;

		SLKC_API BinaryExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API BinaryExprNode(const BinaryExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~BinaryExprNode();
	};

	class TernaryExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> cond, lhs, rhs;

		SLKC_API TernaryExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API TernaryExprNode(const TernaryExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~TernaryExprNode();
	};

	class IdRefExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		IdRefPtr id_ref_ptr;

		SLKC_API IdRefExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, IdRefPtr &&id_ref_ptr);
		SLKC_API IdRefExprNode(const IdRefExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~IdRefExprNode();
	};

	class LooseIdExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::String id;

		SLKC_API LooseIdExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, peff::String &&id);
		SLKC_API LooseIdExprNode(const LooseIdExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~LooseIdExprNode();
	};

	class HeadedIdRefExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> head;
		IdRefPtr id_ref_ptr;

		SLKC_API HeadedIdRefExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &head, IdRefPtr &&id_ref_ptr);
		SLKC_API HeadedIdRefExprNode(const HeadedIdRefExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~HeadedIdRefExprNode();
	};

	class I8LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		int8_t data;

		struct GetData {
			SLAKE_FORCEINLINE int8_t operator()(const AstNodePtr<I8LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<I8LiteralExprNode> l, int8_t data) const {
				l->data = data;
			}
		};

		SLKC_API I8LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, int8_t data);
		SLKC_API I8LiteralExprNode(const I8LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~I8LiteralExprNode();
	};

	class I16LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		int16_t data;

		struct GetData {
			SLAKE_FORCEINLINE int16_t operator()(const AstNodePtr<I16LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<I16LiteralExprNode> l, int16_t data) const {
				l->data = data;
			}
		};

		SLKC_API I16LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, int16_t data);
		SLKC_API I16LiteralExprNode(const I16LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~I16LiteralExprNode();
	};

	class I32LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		int32_t data;

		struct GetData {
			SLAKE_FORCEINLINE int32_t operator()(const AstNodePtr<I32LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<I32LiteralExprNode> l, int32_t data) const {
				l->data = data;
			}
		};

		SLKC_API I32LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, int32_t data);
		SLKC_API I32LiteralExprNode(const I32LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~I32LiteralExprNode();
	};

	class I64LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		int64_t data;

		struct GetData {
			SLAKE_FORCEINLINE int64_t operator()(const AstNodePtr<I64LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<I64LiteralExprNode> l, int64_t data) const {
				l->data = data;
			}
		};

		SLKC_API I64LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, int64_t data);
		SLKC_API I64LiteralExprNode(const I64LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~I64LiteralExprNode();
	};

	class U8LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		uint8_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint8_t operator()(const AstNodePtr<U8LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<U8LiteralExprNode> l, uint8_t data) const {
				l->data = data;
			}
		};

		SLKC_API U8LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, uint8_t data);
		SLKC_API U8LiteralExprNode(const U8LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~U8LiteralExprNode();
	};

	class U16LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		uint16_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint16_t operator()(const AstNodePtr<U16LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<U16LiteralExprNode> l, uint16_t data) const {
				l->data = data;
			}
		};

		SLKC_API U16LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, uint16_t data);
		SLKC_API U16LiteralExprNode(const U16LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~U16LiteralExprNode();
	};

	class U32LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		uint32_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint32_t operator()(const AstNodePtr<U32LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<U32LiteralExprNode> l, uint32_t data) const {
				l->data = data;
			}
		};

		SLKC_API U32LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, uint32_t data);
		SLKC_API U32LiteralExprNode(const U32LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~U32LiteralExprNode();
	};

	class U64LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		uint64_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint64_t operator()(const AstNodePtr<U64LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<U64LiteralExprNode> l, uint64_t data) const {
				l->data = data;
			}
		};

		SLKC_API U64LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, uint64_t data);
		SLKC_API U64LiteralExprNode(const U64LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~U64LiteralExprNode();
	};

	class F32LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		float data;

		struct GetData {
			SLAKE_FORCEINLINE float operator()(const AstNodePtr<F32LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<F32LiteralExprNode> l, float data) const {
				l->data = data;
			}
		};

		SLKC_API F32LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, float data);
		SLKC_API F32LiteralExprNode(const F32LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~F32LiteralExprNode();
	};

	class F64LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		double data;

		struct GetData {
			SLAKE_FORCEINLINE double operator()(const AstNodePtr<F64LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<F64LiteralExprNode> l, double data) const {
				l->data = data;
			}
		};

		SLKC_API F64LiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, double data);
		SLKC_API F64LiteralExprNode(const F64LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~F64LiteralExprNode();
	};

	class BoolLiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		bool data;

		struct GetData {
			SLAKE_FORCEINLINE bool operator()(const AstNodePtr<BoolLiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(AstNodePtr<BoolLiteralExprNode> l, bool data) const {
				l->data = data;
			}
		};

		SLKC_API BoolLiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, bool data);
		SLKC_API BoolLiteralExprNode(const BoolLiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~BoolLiteralExprNode();
	};

	class StringLiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::String data;

		SLKC_API StringLiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, peff::String &&data);
		SLKC_API StringLiteralExprNode(const StringLiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~StringLiteralExprNode();
	};

	class NullLiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API NullLiteralExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API NullLiteralExprNode(const NullLiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~NullLiteralExprNode();
	};

	class InitializerListExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<ExprNode>> elements;

		SLKC_API InitializerListExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API InitializerListExprNode(const InitializerListExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~InitializerListExprNode();
	};

	class CallExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> target;
		peff::DynArray<AstNodePtr<ExprNode>> args;
		AstNodePtr<ExprNode> with_object;
		peff::DynArray<size_t> idx_comma_tokens;
		bool is_async = false;
		size_t l_parenthese_token_index = SIZE_MAX, r_parenthese_token_index = SIZE_MAX, async_keyword_token_index = SIZE_MAX;

		SLKC_API CallExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &target, peff::DynArray<AstNodePtr<ExprNode>> &&args);
		SLKC_API CallExprNode(const CallExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~CallExprNode();
	};

	class NewExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> target_type;
		peff::DynArray<AstNodePtr<ExprNode>> args;
		peff::DynArray<size_t> idx_comma_tokens;
		size_t l_parenthese_token_index = SIZE_MAX, r_parenthese_token_index = SIZE_MAX, async_keyword_token_index = SIZE_MAX;

		SLKC_API NewExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API NewExprNode(const NewExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~NewExprNode();
	};

	class AllocaExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> target_type;
		AstNodePtr<ExprNode> count_expr;
		peff::DynArray<size_t> idx_comma_tokens;
		size_t l_parenthese_token_index = SIZE_MAX, r_parenthese_token_index = SIZE_MAX, async_keyword_token_index = SIZE_MAX;

		SLKC_API AllocaExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API AllocaExprNode(const AllocaExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~AllocaExprNode();
	};

	class CastExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> target_type;
		AstNodePtr<ExprNode> source;
		size_t nullable_token_index = SIZE_MAX, as_keyword_token_index = SIZE_MAX;

		SLKC_API CastExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API CastExprNode(const CastExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~CastExprNode();

		SLAKE_FORCEINLINE bool is_nullable_cast() const noexcept {
			return nullable_token_index != SIZE_MAX;
		}
	};

	class MatchExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> condition;
		AstNodePtr<TypeNameNode> return_type;
		peff::DynArray<std::pair<AstNodePtr<ExprNode>, AstNodePtr<ExprNode>>> cases;

		SLKC_API MatchExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API MatchExprNode(const MatchExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~MatchExprNode();
	};

	class WrapperExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> target;

		SLKC_API WrapperExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API WrapperExprNode(const WrapperExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~WrapperExprNode();
	};

	class RegIndexExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		uint32_t reg;
		AstNodePtr<TypeNameNode> type;

		SLKC_API RegIndexExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, uint32_t reg, AstNodePtr<TypeNameNode> type);
		SLKC_API RegIndexExprNode(const RegIndexExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~RegIndexExprNode();
	};

	class TypeNameExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> type;

		SLKC_API TypeNameExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, AstNodePtr<TypeNameNode> type);
		SLKC_API TypeNameExprNode(const TypeNameExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~TypeNameExprNode();
	};

	class BadExprNode : public ExprNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> incomplete_expr;

		SLKC_API BadExprNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &incomplete_expr);
		SLKC_API BadExprNode(const BadExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~BadExprNode();
	};
}

#endif
