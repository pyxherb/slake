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

			Bad,	 // Bad statement - unrecognized statement type
		};

		class StmtNode : public AstNode {
		public:
			virtual ~StmtNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Stmt; }

			virtual StmtType getStmtType() const = 0;
		};

		template <StmtType st>
		class SimpleStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			inline SimpleStmtNode(Location loc) : _loc(loc) {}
			virtual ~SimpleStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return st; }
		};

		using BreakStmtNode = SimpleStmtNode<StmtType::Break>;
		using ContinueStmtNode = SimpleStmtNode<StmtType::Continue>;

		class BadStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<StmtNode> body;

			inline BadStmtNode(
				Location loc,
				shared_ptr<StmtNode> body)
				: _loc(loc),
				  body(body) {}
			virtual ~BadStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Bad; }
		};

		class ExprStmtNode : public StmtNode {
		public:
			shared_ptr<ExprNode> expr;

			inline ExprStmtNode() {}
			virtual ~ExprStmtNode() = default;

			virtual inline Location getLocation() const override { return expr->getLocation(); }

			virtual inline StmtType getStmtType() const override { return StmtType::Expr; }
		};

		struct VarDefEntry {
			string name;
			Location loc;
			shared_ptr<ExprNode> initValue;
			shared_ptr<TypeNameNode> type;

			size_t idxNameToken;

			inline VarDefEntry() = default;
			inline VarDefEntry(const VarDefEntry &) = default;
			inline VarDefEntry(
				Location loc,
				string name,
				shared_ptr<TypeNameNode> type,
				shared_ptr<ExprNode> initValue,
				size_t idxNameToken)
				: loc(loc), name(name), initValue(initValue), type(type), idxNameToken(idxNameToken) {}

			inline VarDefEntry &operator=(const VarDefEntry &) = default;
		};

		class VarDefStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			unordered_map<string, VarDefEntry> varDefs;

			inline VarDefStmtNode(Location loc) : _loc(loc) {}
			virtual ~VarDefStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::VarDef; }
		};

		class BlockStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			deque<shared_ptr<StmtNode>> stmts;

			inline BlockStmtNode(
				Location loc,
				deque<shared_ptr<StmtNode>> stmts)
				: _loc(loc), stmts(stmts) {}
			virtual ~BlockStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};

		class ForStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<VarDefStmtNode> varDefs;
			shared_ptr<ExprNode> condition;
			shared_ptr<ExprNode> endExpr;
			shared_ptr<StmtNode> body;

			inline ForStmtNode(Location loc) : _loc(loc) {}
			virtual ~ForStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::For; }
		};

		class WhileStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> condition;
			shared_ptr<StmtNode> body;

			inline WhileStmtNode(Location loc) : _loc(loc) {}
			virtual ~WhileStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::While; }
		};

		class ReturnStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> returnValue;

			inline ReturnStmtNode(Location loc) : _loc(loc) {}
			virtual ~ReturnStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::Return; }
		};

		class YieldStmtNode : public ReturnStmtNode {
		public:
			inline YieldStmtNode(Location loc) : ReturnStmtNode(loc) {}
			virtual ~YieldStmtNode() = default;

			virtual inline StmtType getStmtType() const override { return StmtType::Yield; }
		};

		class IfStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> condition;
			shared_ptr<StmtNode> body;
			shared_ptr<StmtNode> elseBranch;

			inline IfStmtNode(Location loc) : _loc(loc) {
			}
			virtual ~IfStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::If; }
		};

		struct CatchBlock {
			Location loc;
			shared_ptr<TypeNameNode> targetType;
			string exceptionVarName;
			shared_ptr<StmtNode> body;

			inline CatchBlock(
				Location loc,
				shared_ptr<TypeNameNode> targetType,
				string exceptionVarName,
				shared_ptr<StmtNode> body)
				: loc(loc),
				  targetType(targetType),
				  exceptionVarName(exceptionVarName),
				  body(body) {}
		};

		struct FinalBlock {
			Location loc;
			shared_ptr<StmtNode> body;

			FinalBlock() = default;
			inline FinalBlock(Location loc) : loc(loc) {}
		};

		class TryStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<StmtNode> body;
			deque<CatchBlock> catchBlocks;
			FinalBlock finalBlock;

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
			deque<shared_ptr<StmtNode>> stmts;
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
			shared_ptr<ExprNode> condition;
			deque<shared_ptr<StmtNode>> body;

			inline SwitchCase(
				Location loc,
				deque<shared_ptr<StmtNode>> body,
				shared_ptr<ExprNode> condition = {})
				: loc(loc),
				  condition(condition),
				  body(body) {
			}
		};

		class SwitchStmtNode : public StmtNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> expr;
			deque<SwitchCase> cases;

			inline SwitchStmtNode(Location loc) : _loc(loc) {}
			virtual ~SwitchStmtNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline StmtType getStmtType() const override { return StmtType::CodeBlock; }
		};
	}
}

#endif
