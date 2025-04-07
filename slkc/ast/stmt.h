#ifndef _SLKC_AST_STMT_H_
#define _SLKC_AST_STMT_H_

#include "expr.h"

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
		Switch,		// Switch
		CodeBlock,	// Code block
		Goto,		// Goto
		Defer,		// Defer

		Bad,  // Bad statement - unrecognized statement type
	};

	class StmtNode : public AstNode {
	public:
		StmtKind stmtKind;

		SLKC_API StmtNode(StmtKind stmtKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API StmtNode(const StmtNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~StmtNode();
	};

	class ExprStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> expr;

		SLKC_API ExprStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ExprStmtNode(const ExprStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ExprStmtNode();
	};

	class DeferStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> expr;

		SLKC_API DeferStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API DeferStmtNode(const DeferStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~DeferStmtNode();
	};

	class VarDefEntry {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::String name;
		peff::SharedPtr<TypeNameNode> type;
		peff::SharedPtr<ExprNode> initialValue;

		SLKC_API VarDefEntry(peff::Alloc *selfAllocator, peff::String &&name, const peff::SharedPtr<TypeNameNode> &type, const peff::SharedPtr<ExprNode> &initialValue);
		SLKC_API virtual ~VarDefEntry();

		SLKC_API void dealloc() noexcept;
	};

	using VarDefEntryPtr = std::unique_ptr<VarDefEntry, peff::DeallocableDeleter<VarDefEntry>>;

	SLKC_API VarDefEntryPtr duplicateVarDefEntry(VarDefEntry *varDefEntry, peff::Alloc *allocator);

	class VarDefStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<VarDefEntryPtr> varDefEntries;

		SLKC_API VarDefStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::DynArray<VarDefEntryPtr> &&varDefEntries);
		SLKC_API VarDefStmtNode(const VarDefStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~VarDefStmtNode();
	};

	class BreakStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API BreakStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API BreakStmtNode(const BreakStmtNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~BreakStmtNode();
	};

	class ContinueStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		SLKC_API ContinueStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ContinueStmtNode(const ContinueStmtNode &rhs, peff::Alloc *allocator);
		SLKC_API virtual ~ContinueStmtNode();
	};

	class ForStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<VarDefEntryPtr> varDefEntries;
		peff::SharedPtr<ExprNode> cond, step;
		peff::SharedPtr<StmtNode> body;

		SLKC_API ForStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ForStmtNode(const ForStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ForStmtNode();
	};

	class ForEachStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::String varName;
		peff::SharedPtr<ExprNode> cond;
		peff::SharedPtr<StmtNode> body;

		SLKC_API ForEachStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&varName, const peff::SharedPtr<ExprNode> &cond, const peff::SharedPtr<StmtNode> &body);
		SLKC_API ForEachStmtNode(const ForEachStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ForEachStmtNode();
	};

	class WhileStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> cond;
		peff::SharedPtr<StmtNode> body;
		bool isDoWhile = false;

		SLKC_API WhileStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API WhileStmtNode(const WhileStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~WhileStmtNode();
	};

	class ReturnStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> value;

		SLKC_API ReturnStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &value);
		SLKC_API ReturnStmtNode(const ReturnStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ReturnStmtNode();
	};

	class YieldStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> value;

		SLKC_API YieldStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &value);
		SLKC_API YieldStmtNode(const YieldStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~YieldStmtNode();
	};

	class IfStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<ExprNode> cond;
		peff::SharedPtr<StmtNode> trueBody, falseBody;

		SLKC_API IfStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<ExprNode> &cond, const peff::SharedPtr<StmtNode> &trueBody, const peff::SharedPtr<StmtNode> &falseBody);
		SLKC_API IfStmtNode(const IfStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~IfStmtNode();
	};

	class CodeBlockStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::DynArray<peff::SharedPtr<StmtNode>> body;

		SLKC_API CodeBlockStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API CodeBlockStmtNode(const CodeBlockStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~CodeBlockStmtNode();
	};

	class BadStmtNode : public StmtNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<StmtNode> body;

		SLKC_API BadStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const peff::SharedPtr<StmtNode> &body);
		SLKC_API BadStmtNode(const BadStmtNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~BadStmtNode();
	};
}

#endif
