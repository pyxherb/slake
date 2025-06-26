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

		RegRef,	 // Register reference

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
		ExprKind exprKind;

		SLKC_API ExprNode(ExprKind exprKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ExprNode(const ExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~ExprNode();
	};

	class UnaryExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		UnaryOp unaryOp;
		peff::SharedPtr<ExprNode> operand;

		SLKC_API UnaryExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API UnaryExprNode(const UnaryExprNode &rhs, peff::Alloc *selfAllocator, bool &succeededOut);
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

	SLKC_API const char *getBinaryOperatorOverloadingName(BinaryOp op);

	class BinaryExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		BinaryOp binaryOp;
		peff::SharedPtr<ExprNode> lhs, rhs;

		SLKC_API BinaryExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API BinaryExprNode(const BinaryExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~BinaryExprNode();
	};

	class TernaryExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> cond, lhs, rhs;

		SLKC_API TernaryExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API TernaryExprNode(const TernaryExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~TernaryExprNode();
	};

	class IdRefExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		IdRefPtr idRefPtr;

		SLKC_API IdRefExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, IdRefPtr &&idRefPtr);
		SLKC_API IdRefExprNode(const IdRefExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~IdRefExprNode();
	};

	class LooseIdExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::String id;

		SLKC_API LooseIdExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&id);
		SLKC_API LooseIdExprNode(const LooseIdExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~LooseIdExprNode();
	};

	class HeadedIdRefExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> head;
		IdRefPtr idRefPtr;

		SLKC_API HeadedIdRefExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &head, IdRefPtr &&idRefPtr);
		SLKC_API HeadedIdRefExprNode(const HeadedIdRefExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~HeadedIdRefExprNode();
	};

	class I8LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		int8_t data;

		struct GetData {
			SLAKE_FORCEINLINE int8_t operator()(const peff::SharedPtr<I8LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<I8LiteralExprNode> l, int8_t data) const {
				l->data = data;
			}
		};

		SLKC_API I8LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, int8_t data);
		SLKC_API I8LiteralExprNode(const I8LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~I8LiteralExprNode();
	};

	class I16LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		int16_t data;

		struct GetData {
			SLAKE_FORCEINLINE int16_t operator()(const peff::SharedPtr<I16LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<I16LiteralExprNode> l, int16_t data) const {
				l->data = data;
			}
		};

		SLKC_API I16LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, int16_t data);
		SLKC_API I16LiteralExprNode(const I16LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~I16LiteralExprNode();
	};

	class I32LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		int32_t data;

		struct GetData {
			SLAKE_FORCEINLINE int32_t operator()(const peff::SharedPtr<I32LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<I32LiteralExprNode> l, int32_t data) const {
				l->data = data;
			}
		};

		SLKC_API I32LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, int32_t data);
		SLKC_API I32LiteralExprNode(const I32LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~I32LiteralExprNode();
	};

	class I64LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		int64_t data;

		struct GetData {
			SLAKE_FORCEINLINE int64_t operator()(const peff::SharedPtr<I64LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<I64LiteralExprNode> l, int64_t data) const {
				l->data = data;
			}
		};

		SLKC_API I64LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, int64_t data);
		SLKC_API I64LiteralExprNode(const I64LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~I64LiteralExprNode();
	};

	class U8LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		uint8_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint8_t operator()(const peff::SharedPtr<U8LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<U8LiteralExprNode> l, uint8_t data) const {
				l->data = data;
			}
		};

		SLKC_API U8LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, uint8_t data);
		SLKC_API U8LiteralExprNode(const U8LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~U8LiteralExprNode();
	};

	class U16LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		uint16_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint16_t operator()(const peff::SharedPtr<U16LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<U16LiteralExprNode> l, uint16_t data) const {
				l->data = data;
			}
		};

		SLKC_API U16LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, uint16_t data);
		SLKC_API U16LiteralExprNode(const U16LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~U16LiteralExprNode();
	};

	class U32LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		uint32_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint32_t operator()(const peff::SharedPtr<U32LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<U32LiteralExprNode> l, uint32_t data) const {
				l->data = data;
			}
		};

		SLKC_API U32LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, uint32_t data);
		SLKC_API U32LiteralExprNode(const U32LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~U32LiteralExprNode();
	};

	class U64LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		uint64_t data;

		struct GetData {
			SLAKE_FORCEINLINE uint64_t operator()(const peff::SharedPtr<U64LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<U64LiteralExprNode> l, uint64_t data) const {
				l->data = data;
			}
		};

		SLKC_API U64LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, uint64_t data);
		SLKC_API U64LiteralExprNode(const U64LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~U64LiteralExprNode();
	};

	class F32LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		float data;

		struct GetData {
			SLAKE_FORCEINLINE float operator()(const peff::SharedPtr<F32LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<F32LiteralExprNode> l, float data) const {
				l->data = data;
			}
		};

		SLKC_API F32LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, float data);
		SLKC_API F32LiteralExprNode(const F32LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~F32LiteralExprNode();
	};

	class F64LiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		double data;

		struct GetData {
			SLAKE_FORCEINLINE double operator()(const peff::SharedPtr<F64LiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<F64LiteralExprNode> l, double data) const {
				l->data = data;
			}
		};

		SLKC_API F64LiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, double data);
		SLKC_API F64LiteralExprNode(const F64LiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~F64LiteralExprNode();
	};

	class BoolLiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		bool data;

		struct GetData {
			SLAKE_FORCEINLINE bool operator()(const peff::SharedPtr<BoolLiteralExprNode> &l) const {
				return l->data;
			}
		};

		struct SetData {
			SLAKE_FORCEINLINE void operator()(peff::SharedPtr<BoolLiteralExprNode> l, bool data) const {
				l->data = data;
			}
		};

		SLKC_API BoolLiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, bool data);
		SLKC_API BoolLiteralExprNode(const BoolLiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~BoolLiteralExprNode();
	};

	class StringLiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::String data;

		SLKC_API StringLiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&data);
		SLKC_API StringLiteralExprNode(const StringLiteralExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~StringLiteralExprNode();
	};

	class NullLiteralExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API NullLiteralExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API NullLiteralExprNode(const NullLiteralExprNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~NullLiteralExprNode();
	};

	class InitializerListExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<peff::SharedPtr<ExprNode>> elements;

		SLKC_API InitializerListExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API InitializerListExprNode(const InitializerListExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~InitializerListExprNode();
	};

	class CallExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> target;
		peff::DynArray<peff::SharedPtr<ExprNode>> args;
		peff::SharedPtr<ExprNode> withObject;
		peff::DynArray<size_t> idxCommaTokens;
		bool isAsync = false;
		size_t lParentheseTokenIndex = SIZE_MAX, rParentheseTokenIndex = SIZE_MAX, asyncKeywordTokenIndex = SIZE_MAX;

		SLKC_API CallExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &target, peff::DynArray<peff::SharedPtr<ExprNode>> &&args);
		SLKC_API CallExprNode(const CallExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~CallExprNode();
	};

	class NewExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<TypeNameNode> targetType;
		peff::DynArray<peff::SharedPtr<ExprNode>> args;
		peff::DynArray<size_t> idxCommaTokens;
		size_t lParentheseTokenIndex = SIZE_MAX, rParentheseTokenIndex = SIZE_MAX, asyncKeywordTokenIndex = SIZE_MAX;

		SLKC_API NewExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API NewExprNode(const NewExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~NewExprNode();
	};

	class AllocaExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<TypeNameNode> targetType;
		peff::SharedPtr<ExprNode> countExpr;
		peff::DynArray<size_t> idxCommaTokens;
		size_t lParentheseTokenIndex = SIZE_MAX, rParentheseTokenIndex = SIZE_MAX, asyncKeywordTokenIndex = SIZE_MAX;

		SLKC_API AllocaExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API AllocaExprNode(const AllocaExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~AllocaExprNode();
	};

	class CastExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<TypeNameNode> targetType;
		peff::SharedPtr<ExprNode> source;
		size_t asKeywordTokenIndex = SIZE_MAX;

		SLKC_API CastExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API CastExprNode(const CastExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~CastExprNode();
	};

	class MatchExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> condition;
		bool isConst = false;
		peff::SharedPtr<TypeNameNode> returnType;
		peff::DynArray<std::pair<peff::SharedPtr<ExprNode>, peff::SharedPtr<ExprNode>>> cases;

		SLKC_API MatchExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API MatchExprNode(const MatchExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~MatchExprNode();
	};

	class WrapperExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> target;

		SLKC_API WrapperExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &target);
		SLKC_API WrapperExprNode(const WrapperExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~WrapperExprNode();
	};

	class RegRefExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		uint32_t reg;
		peff::SharedPtr<TypeNameNode> type;

		SLKC_API RegRefExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, uint32_t reg, peff::SharedPtr<TypeNameNode> type);
		SLKC_API RegRefExprNode(const RegRefExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~RegRefExprNode();
	};

	class BadExprNode : public ExprNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> incompleteExpr;

		SLKC_API BadExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &incompleteExpr);
		SLKC_API BadExprNode(const BadExprNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~BadExprNode();
	};
}

#endif
