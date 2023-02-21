#ifndef _SLKC_COMPILER_COMPILER_HH
#define _SLKC_COMPILER_COMPILER_HH

#include <slake/slxfmt.h>

#include <fstream>

#include "init.hh"

namespace Slake {
	namespace Compiler {
		struct LocalVar {
			std::uint32_t stackPos;
			std::shared_ptr<TypeName> type;

			inline LocalVar& operator=(const LocalVar& x) {
				stackPos = x.stackPos;
				type = x.type;
				return *this;
			}

			inline LocalVar& operator=(const LocalVar&& x) {
				stackPos = x.stackPos;
				type = x.type;
				return *this;
			}

			inline LocalVar(std::uint32_t stackPos, std::shared_ptr<TypeName> type) {
				this->stackPos = stackPos;
				this->type = type;
			}
			inline LocalVar() {}
		};

		struct Ins {
			Opcode opcode;
			std::vector<std::shared_ptr<Expr>> operands;

			inline Ins(Opcode opcode, std::initializer_list<std::shared_ptr<Expr>> operands) {
				this->opcode = opcode;
				this->operands = operands;
			}
		};

		struct Fn {
			std::vector<Ins> body;
			std::unordered_map<std::string, std::uint32_t> labels;
			std::unordered_map<std::string, LocalVar> localVars;

			inline void insertLabel(std::string name) { labels[name] = body.size(); };
		};

		struct State {
			std::unordered_map<std::string, std::shared_ptr<Fn>> fnDefs;
			std::shared_ptr<Scope> scope;
			std::uint32_t stackCur, nContinueLevel, nBreakLevel;
			std::string currentFn;

			inline void enterLoop() noexcept {
				nContinueLevel++, nBreakLevel++;
			}

			inline void leaveLoop() noexcept {
				nContinueLevel--, nBreakLevel--;
			}

			inline void enterSwitch() noexcept {
				nContinueLevel++;
			}

			inline void leaveSwitch() noexcept {
				nContinueLevel--;
			}
		};

		std::shared_ptr<TypeName> evalExprType(std::shared_ptr<State> state, std::shared_ptr<Expr> expr, bool isRecursing = false);
		inline std::shared_ptr<Expr> evalConstExpr(std::shared_ptr<Expr> expr) {
			switch (expr->getType()) {
				case ExprType::LITERAL:
					return expr;
				case ExprType::UNARY: {
					std::shared_ptr<UnaryOpExpr> opExpr = std::static_pointer_cast<UnaryOpExpr>(expr);
					switch (opExpr->x->getType()) {
						case ExprType::LITERAL:
							return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execUnaryOp(opExpr->op);
						case ExprType::REF: {
							auto ref = std::static_pointer_cast<RefExpr>(opExpr->x);
							//
							// Variable and function are both not evaluatable at compile time.
							//
							if ((currentScope->getVar(ref->name)) || (currentScope->getFn(ref->name)))
								return std::shared_ptr<Expr>();
							{
								auto x = currentScope->getEnumItem(ref);
								if (x)
									return evalConstExpr(x);
							}
							break;
						}
					}
					break;
				}
				case ExprType::BINARY: {
					std::shared_ptr<BinaryOpExpr> opExpr = std::static_pointer_cast<BinaryOpExpr>(expr);
					auto x = evalConstExpr(opExpr->x);
					if ((!x) || (x->getType() != ExprType::LITERAL))
						return std::shared_ptr<Expr>();
					auto y = evalConstExpr(opExpr->y);
					if ((!y) || (y->getType() != ExprType::LITERAL))
						return std::shared_ptr<Expr>();
					return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execBinaryOp(opExpr->op, std::static_pointer_cast<LiteralExpr>(opExpr->y));
				}
				default:
					return std::shared_ptr<Expr>();
			}
			return std::shared_ptr<Expr>();
		}

		inline void writeInsHeader(SlxFmt::InsHeader ih, std::fstream& fs) {
			fs.write((char*)&ih, sizeof(ih));
		}
		void writeIns(Opcode opcode, std::fstream& fs, std::initializer_list<std::shared_ptr<Expr>> operands = {});
		void compileExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, std::string fnName, bool isRecursing = false);
		void compileStmt(std::shared_ptr<Compiler::Stmt> src, std::shared_ptr<State> s, std::string fnName);
		void compile(std::shared_ptr<Scope> scope, std::fstream& fs, bool isTopLevel = true);
	}
}

#endif
