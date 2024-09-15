///
/// @file stmt.h
/// @brief Header file which handles definitions about statements.
///
/// @copyright Copyright (c) 2023 Slake contributors
///
///
#ifndef _SLKC_COMPILER_AST_STMT_H_
#define _SLKC_COMPILER_AST_STMT_H_

#include "astnode.h"
#include "expr.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		enum class StmtType : uint8_t {
			Expr = 0,	// Expression
			VarDef,		// (Local) Variable definition
			Break,		// Break
			Continue,	// Continue
			For,		// For
			While,		// While
			Return,		// Return
			Yield,		// Yield
			If,			// If
			Try,		// Try
			Switch,		// Switch
			CodeBlock,	// Code block

			Bad,  // Bad statement - unrecognized statement type
		};

		class StmtNode : public AstNode {
		public:
			virtual ~StmtNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Stmt; }

			virtual StmtType getStmtType() const = 0;
		};

		class BreakStmtNode : public StmtNode {
		public:
			size_t idxBreakToken = SIZE_MAX, idxSemicolonToken = SIZE_MAX;

			inline BreakStmtNode() {}
			virtual ~BreakStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Break; }
		};

		class ContinueStmtNode : public StmtNode {
		public:
			size_t idxContinueToken = SIZE_MAX, idxSemicolonToken = SIZE_MAX;
			inline ContinueStmtNode() {}
			virtual ~ContinueStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Break; }
		};

		class BadStmtNode : public StmtNode {
		public:
			std::shared_ptr<StmtNode> body;

			inline BadStmtNode(
				std::shared_ptr<StmtNode> body)
				: body(body) {}
			virtual ~BadStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Bad; }
		};

		class ExprStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> expr;
			size_t idxSemicolonToken = SIZE_MAX;

			inline ExprStmtNode() {}
			virtual ~ExprStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Expr; }
		};

		struct VarDefEntry {
			std::string name;
			TokenRange tokenRange;
			std::shared_ptr<ExprNode> initValue;
			std::shared_ptr<TypeNameNode> type;

			size_t idxNameToken = SIZE_MAX,
				   idxColonToken = SIZE_MAX,
				   idxAssignOpToken = SIZE_MAX,
				idxCommaToken = SIZE_MAX;

			VarDefEntry() = default;
			inline VarDefEntry(TokenRange tokenRange, std::string name, size_t idxNameToken) : tokenRange(tokenRange), name(name), idxNameToken(idxNameToken) {}
		};

		class VarDefStmtNode : public StmtNode {
		public:
			std::unordered_map<std::string, VarDefEntry> varDefs;
			size_t idxLetToken = SIZE_MAX,
				   idxSemicolonToken = SIZE_MAX;

			inline VarDefStmtNode() {}
			virtual ~VarDefStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::VarDef; }
		};

		class ForStmtNode : public StmtNode {
		public:
			std::shared_ptr<VarDefStmtNode> varDefs;
			std::shared_ptr<ExprNode> condition;
			std::shared_ptr<ExprNode> endExpr;
			std::shared_ptr<StmtNode> body;

			size_t idxForToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxFirstSemicolonToken = SIZE_MAX,
				   idxSecondSemicolonToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;

			inline ForStmtNode() {}
			virtual ~ForStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::For; }
		};

		class WhileStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> condition;
			std::shared_ptr<StmtNode> body;

			size_t idxWhileToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;

			inline WhileStmtNode() {}
			virtual ~WhileStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::While; }
		};

		class ReturnStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> returnValue;

			size_t idxReturnToken = SIZE_MAX,
				   idxSemicolonToken = SIZE_MAX;

			inline ReturnStmtNode() {}
			virtual ~ReturnStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Return; }
		};

		class YieldStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> returnValue;

			size_t idxYieldToken = SIZE_MAX,
				   idxSemicolonToken = SIZE_MAX;

			inline YieldStmtNode() {}
			virtual ~YieldStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Yield; }
		};

		class IfStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> condition;
			std::shared_ptr<StmtNode> body;
			std::shared_ptr<StmtNode> elseBranch;

			size_t idxIfToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX,
				   idxElseToken = SIZE_MAX;

			inline IfStmtNode() {
			}
			virtual ~IfStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::If; }
		};

		struct CatchBlock {
			TokenRange tokenRange;
			std::shared_ptr<TypeNameNode> targetType;
			std::string exceptionVarName;
			std::shared_ptr<StmtNode> body;

			size_t idxCatchToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxExceptionVarNameToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;
		};

		struct FinalBlock {
			TokenRange tokenRange;
			std::shared_ptr<StmtNode> body;

			size_t idxFinalToken = SIZE_MAX;
		};

		class TryStmtNode : public StmtNode {
		public:
			std::shared_ptr<StmtNode> body;
			std::deque<CatchBlock> catchBlocks;
			FinalBlock finalBlock;

			size_t idxTryToken = SIZE_MAX;

			inline TryStmtNode() {
			}
			virtual ~TryStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Try; }
		};

		struct CodeBlock {
			TokenRange tokenRange;
			std::deque<std::shared_ptr<StmtNode>> stmts;

			size_t idxLBraceToken = SIZE_MAX, idxRBraceToken = SIZE_MAX;
		};

		class CodeBlockStmtNode : public StmtNode {
		public:
			CodeBlock body;

			inline CodeBlockStmtNode(CodeBlock body) : body(body) {}
			virtual ~CodeBlockStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};

		struct SwitchCase {
			TokenRange tokenRange;
			std::shared_ptr<ExprNode> condition;
			std::deque<std::shared_ptr<StmtNode>> body;

			size_t idxCaseToken = SIZE_MAX,
				   idxColonToken = SIZE_MAX;
		};

		class SwitchStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> expr;
			std::deque<SwitchCase> cases;

			size_t idxSwitchToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX,
				   idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			inline SwitchStmtNode() {}
			virtual ~SwitchStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};
	}
}

#endif
