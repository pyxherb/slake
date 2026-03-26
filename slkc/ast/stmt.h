#ifndef _SLKC_AST_STMT_H_
#define _SLKC_AST_STMT_H_

#include "expr.h"
#include "generic.h"

namespace slkc {
	enum class StmtKind : uint8_t {
		Expr = 0,	// Expression
		VarDef,		// Variable definition
		Break,		// Break
		Continue,	// Continue
		For,		// For
		ForEach,	// For each
		While,		// While
		DoWhile,	// Do while
		Return,		// Return
		Yield,		// Yield
		If,			// If
		With,		// With
		Switch,		// Switch
		CaseLabel,	// Case label
		CodeBlock,	// Code block
		Goto,		// Goto
		Label,		// Label

		Bad,  // Bad statement - unrecognized statement type
	};

	class StmtNode : public AstNode {
	public:
		StmtKind stmt_kind;

		SLKC_API StmtNode(StmtKind stmt_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API StmtNode(const StmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~StmtNode();
	};

	class ExprStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> expr;

		SLKC_API ExprStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ExprStmtNode(const ExprStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ExprStmtNode();
	};

	class VarDefEntry {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		peff::String name;
		AstNodePtr<TypeNameNode> type;
		AstNodePtr<ExprNode> initial_value;
		peff::DynArray<AstNodePtr<AttributeNode>> attributes;
		size_t idx_name_token = SIZE_MAX;

		SLKC_API VarDefEntry(peff::Alloc *self_allocator);
		SLKC_API virtual ~VarDefEntry();

		SLKC_API void dealloc() noexcept;
	};

	using VarDefEntryPtr = std::unique_ptr<VarDefEntry, peff::DeallocableDeleter<VarDefEntry>>;

	SLKC_API VarDefEntryPtr duplicate_var_def_entry(VarDefEntry *var_def_entry, peff::Alloc *allocator);

	class VarDefStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		slake::AccessModifier access_modifier;
		peff::DynArray<VarDefEntryPtr> var_def_entries;

		SLKC_API VarDefStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, peff::DynArray<VarDefEntryPtr> &&var_def_entries);
		SLKC_API VarDefStmtNode(const VarDefStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~VarDefStmtNode();
	};

	class BreakStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API BreakStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API BreakStmtNode(const BreakStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~BreakStmtNode();
	};

	class ContinueStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		SLKC_API ContinueStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ContinueStmtNode(const ContinueStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~ContinueStmtNode();
	};

	class ForStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<VarDefEntryPtr> var_def_entries;
		AstNodePtr<ExprNode> cond, step;
		AstNodePtr<StmtNode> body;

		SLKC_API ForStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ForStmtNode(const ForStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ForStmtNode();
	};

	class ForEachStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::String var_name;
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> body;

		SLKC_API ForEachStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, peff::String &&var_name, const AstNodePtr<ExprNode> &cond, const AstNodePtr<StmtNode> &body);
		SLKC_API ForEachStmtNode(const ForEachStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ForEachStmtNode();
	};

	class WhileStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> body;

		SLKC_API WhileStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API WhileStmtNode(const WhileStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~WhileStmtNode();
	};

	class DoWhileStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> body;

		SLKC_API DoWhileStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API DoWhileStmtNode(const DoWhileStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~DoWhileStmtNode();
	};

	class ReturnStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> value;

		SLKC_API ReturnStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value);
		SLKC_API ReturnStmtNode(const ReturnStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ReturnStmtNode();
	};

	class YieldStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> value;

		SLKC_API YieldStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value);
		SLKC_API YieldStmtNode(const YieldStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~YieldStmtNode();
	};

	class IfStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> true_body, false_body;

		SLKC_API IfStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API IfStmtNode(const IfStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~IfStmtNode();
	};

	class WithConstraintEntry {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		peff::String generic_param_name;
		GenericConstraintPtr constraint;

		SLKC_API WithConstraintEntry(peff::Alloc *self_allocator);
		SLKC_API virtual ~WithConstraintEntry();

		SLKC_API void dealloc() noexcept;
	};
	using WithConstraintEntryPtr = std::unique_ptr<WithConstraintEntry, peff::DeallocableDeleter<WithConstraintEntry>>;

	WithConstraintEntryPtr duplicate_with_constraint_entry(peff::Alloc *allocator, const WithConstraintEntry *constraint);

	class WithStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<WithConstraintEntryPtr> constraints;
		AstNodePtr<StmtNode> true_body, false_body;

		SLKC_API WithStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API WithStmtNode(const WithStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~WithStmtNode();
	};

	class CaseLabelStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> condition;

		SLKC_API CaseLabelStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API CaseLabelStmtNode(const CaseLabelStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~CaseLabelStmtNode();

		SLAKE_FORCEINLINE bool is_default_case() const noexcept {
			return !condition;
		}
	};

	class SwitchStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> condition;
		peff::DynArray<size_t> case_offsets;
		peff::DynArray<AstNodePtr<StmtNode>> body;

		SLKC_API SwitchStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API SwitchStmtNode(const SwitchStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~SwitchStmtNode();
	};

	class LabelStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::String name;

		SLKC_API LabelStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API LabelStmtNode(const LabelStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~LabelStmtNode();
	};

	class CodeBlockStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<StmtNode>> body;

		SLKC_API CodeBlockStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API CodeBlockStmtNode(const CodeBlockStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~CodeBlockStmtNode();
	};

	class BadStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<StmtNode> body;

		SLKC_API BadStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<StmtNode> &body);
		SLKC_API BadStmtNode(const BadStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~BadStmtNode();
	};
}

#endif
