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
		StmtKind stmtKind;

		SLKC_API StmtNode(StmtKind stmtKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API StmtNode(const StmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~StmtNode();
	};

	class ExprStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> expr;

		SLKC_API ExprStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ExprStmtNode(const ExprStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ExprStmtNode();
	};

	class VarDefEntry {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::String name;
		AstNodePtr<TypeNameNode> type;
		AstNodePtr<ExprNode> initialValue;
		peff::DynArray<AstNodePtr<AttributeNode>> attributes;

		SLKC_API VarDefEntry(peff::Alloc *selfAllocator);
		SLKC_API virtual ~VarDefEntry();

		SLKC_API void dealloc() noexcept;
	};

	using VarDefEntryPtr = std::unique_ptr<VarDefEntry, peff::DeallocableDeleter<VarDefEntry>>;

	SLKC_API VarDefEntryPtr duplicateVarDefEntry(VarDefEntry *varDefEntry, peff::Alloc *allocator);

	class VarDefStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		slake::AccessModifier accessModifier;
		peff::DynArray<VarDefEntryPtr> varDefEntries;

		SLKC_API VarDefStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::DynArray<VarDefEntryPtr> &&varDefEntries);
		SLKC_API VarDefStmtNode(const VarDefStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~VarDefStmtNode();
	};

	class BreakStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		SLKC_API BreakStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API BreakStmtNode(const BreakStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~BreakStmtNode();
	};

	class ContinueStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		SLKC_API ContinueStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ContinueStmtNode(const ContinueStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context);
		SLKC_API virtual ~ContinueStmtNode();
	};

	class ForStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<VarDefEntryPtr> varDefEntries;
		AstNodePtr<ExprNode> cond, step;
		AstNodePtr<StmtNode> body;

		SLKC_API ForStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ForStmtNode(const ForStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ForStmtNode();
	};

	class ForEachStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::String varName;
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> body;

		SLKC_API ForEachStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&varName, const AstNodePtr<ExprNode> &cond, const AstNodePtr<StmtNode> &body);
		SLKC_API ForEachStmtNode(const ForEachStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ForEachStmtNode();
	};

	class WhileStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> body;
		bool isDoWhile = false;

		SLKC_API WhileStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API WhileStmtNode(const WhileStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~WhileStmtNode();
	};

	class ReturnStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> value;

		SLKC_API ReturnStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value);
		SLKC_API ReturnStmtNode(const ReturnStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~ReturnStmtNode();
	};

	class YieldStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> value;

		SLKC_API YieldStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value);
		SLKC_API YieldStmtNode(const YieldStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~YieldStmtNode();
	};

	class IfStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> cond;
		AstNodePtr<StmtNode> trueBody, falseBody;

		SLKC_API IfStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API IfStmtNode(const IfStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~IfStmtNode();
	};

	class WithConstraintEntry {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::String genericParamName;
		GenericConstraintPtr constraint;

		SLKC_API WithConstraintEntry(peff::Alloc *selfAllocator);
		SLKC_API virtual ~WithConstraintEntry();

		SLKC_API void dealloc() noexcept;
	};
	using WithConstraintEntryPtr = std::unique_ptr<WithConstraintEntry, peff::DeallocableDeleter<WithConstraintEntry>>;

	WithConstraintEntryPtr duplicateWithConstraintEntry(peff::Alloc *allocator, const WithConstraintEntry *constraint);

	class WithStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<WithConstraintEntryPtr> constraints;
		AstNodePtr<StmtNode> trueBody, falseBody;

		SLKC_API WithStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API WithStmtNode(const WithStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~WithStmtNode();
	};

	class CaseLabelStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> condition;

		SLKC_API CaseLabelStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API CaseLabelStmtNode(const CaseLabelStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~CaseLabelStmtNode();
	};

	class SwitchStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<ExprNode> condition;
		peff::DynArray<size_t> caseOffsets;
		peff::DynArray<AstNodePtr<StmtNode>> body;
		bool isConst = false;

		SLKC_API SwitchStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API SwitchStmtNode(const SwitchStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~SwitchStmtNode();
	};

	class LabelStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::String name;

		SLKC_API LabelStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API LabelStmtNode(const LabelStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~LabelStmtNode();
	};

	class CodeBlockStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		peff::DynArray<AstNodePtr<StmtNode>> body;

		SLKC_API CodeBlockStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API CodeBlockStmtNode(const CodeBlockStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~CodeBlockStmtNode();
	};

	class BadStmtNode : public StmtNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<StmtNode> body;

		SLKC_API BadStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<StmtNode> &body);
		SLKC_API BadStmtNode(const BadStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeededOut);
		SLKC_API virtual ~BadStmtNode();
	};
}

#endif
