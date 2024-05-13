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
		private:
			Location _loc;

		public:
			size_t idxBreakToken = SIZE_MAX, idxSemicolonToken = SIZE_MAX;

			inline BreakStmtNode(
				Location loc)
				: _loc(loc) {}
			virtual ~BreakStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Break; }
		};

		class ContinueStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			size_t idxContinueToken = SIZE_MAX, idxSemicolonToken = SIZE_MAX;
			inline ContinueStmtNode(
				Location loc)
				: _loc(loc) {}
			virtual ~ContinueStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Break; }
		};

		class BadStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<StmtNode> body;

			inline BadStmtNode(
				Location loc,
				std::shared_ptr<StmtNode> body)
				: _loc(loc),
				  body(body) {}
			virtual ~BadStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Bad; }
		};

		class ExprStmtNode : public StmtNode {
		public:
			std::shared_ptr<ExprNode> expr;
			size_t idxSemicolonToken = SIZE_MAX;

			inline ExprStmtNode() {}
			virtual ~ExprStmtNode() = default;

			virtual inline Location getLocation() const override { return expr->getLocation(); }

			virtual inline StmtType getStmtType() const override { return StmtType::Expr; }
		};

		struct VarDefEntry {
			std::string name;
			Location loc;
			std::shared_ptr<ExprNode> initValue;
			std::shared_ptr<TypeNameNode> type;

			size_t idxNameToken = SIZE_MAX,
				   idxColonToken = SIZE_MAX,
				   idxAssignOpToken = SIZE_MAX,
				idxCommaToken = SIZE_MAX;

			VarDefEntry() = default;
			inline VarDefEntry(Location loc, std::string name, size_t idxNameToken) : loc(loc), name(name), idxNameToken(idxNameToken) {}
		};

		class VarDefStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::unordered_map<std::string, VarDefEntry> varDefs;
			size_t idxLetToken = SIZE_MAX,
				   idxSemicolonToken = SIZE_MAX;

			inline VarDefStmtNode(Location loc) : _loc(loc) {}
			virtual ~VarDefStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::VarDef; }
		};

		class BlockStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::deque<std::shared_ptr<StmtNode>> stmts;

			inline BlockStmtNode(
				Location loc,
				std::deque<std::shared_ptr<StmtNode>> stmts)
				: _loc(loc), stmts(stmts) {}
			virtual ~BlockStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};

		class ForStmtNode : public StmtNode {
		private:
			Location _loc;

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

			inline ForStmtNode(Location loc) : _loc(loc) {}
			virtual ~ForStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::For; }
		};

		class WhileStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<ExprNode> condition;
			std::shared_ptr<StmtNode> body;

			size_t idxWhileToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;

			inline WhileStmtNode(Location loc) : _loc(loc) {}
			virtual ~WhileStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::While; }
		};

		class ReturnStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<ExprNode> returnValue;

			size_t idxReturnToken = SIZE_MAX,
				   idxSemicolonToken = SIZE_MAX;

			inline ReturnStmtNode(Location loc) : _loc(loc) {}
			virtual ~ReturnStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Return; }
		};

		class YieldStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<ExprNode> returnValue;

			size_t idxYieldToken = SIZE_MAX,
				   idxSemicolonToken = SIZE_MAX;

			inline YieldStmtNode(Location loc) : _loc(loc) {}
			virtual ~YieldStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Yield; }
		};

		class IfStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<ExprNode> condition;
			std::shared_ptr<StmtNode> body;
			std::shared_ptr<StmtNode> elseBranch;

			size_t idxIfToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX,
				   idxElseToken = SIZE_MAX;

			inline IfStmtNode(Location loc) : _loc(loc) {
			}
			virtual ~IfStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::If; }
		};

		struct CatchBlock {
			Location loc;
			std::shared_ptr<TypeNameNode> targetType;
			std::string exceptionVarName;
			std::shared_ptr<StmtNode> body;

			size_t idxCatchToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxExceptionVarNameToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;
		};

		struct FinalBlock {
			Location loc;
			std::shared_ptr<StmtNode> body;

			size_t idxFinalToken = SIZE_MAX;
		};

		class TryStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<StmtNode> body;
			std::deque<CatchBlock> catchBlocks;
			FinalBlock finalBlock;

			size_t idxTryToken = SIZE_MAX;

			inline TryStmtNode(
				Location loc)
				: _loc(loc) {
			}
			virtual ~TryStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Try; }
		};

		struct CodeBlock {
			Location loc;
			std::deque<std::shared_ptr<StmtNode>> stmts;

			size_t idxLBraceToken = SIZE_MAX, idxRBraceToken = SIZE_MAX;
		};

		class CodeBlockStmtNode : public StmtNode {
		public:
			CodeBlock body;

			inline CodeBlockStmtNode(CodeBlock body) : body(body) {}
			virtual ~CodeBlockStmtNode() = default;

			virtual inline Location getLocation() const override { return body.loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};

		struct SwitchCase {
			Location loc;
			std::shared_ptr<ExprNode> condition;
			std::deque<std::shared_ptr<StmtNode>> body;

			size_t idxCaseToken = SIZE_MAX,
				   idxColonToken = SIZE_MAX;
		};

		class SwitchStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			std::shared_ptr<ExprNode> expr;
			std::deque<SwitchCase> cases;

			size_t idxSwitchToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX,
				   idxLBraceToken = SIZE_MAX,
				   idxRBraceToken = SIZE_MAX;

			inline SwitchStmtNode(Location loc) : _loc(loc) {}
			virtual ~SwitchStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};
	}
}

#endif
