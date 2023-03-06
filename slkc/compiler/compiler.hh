#ifndef _SLKC_COMPILER_COMPILER_HH
#define _SLKC_COMPILER_COMPILER_HH

#include <slake/slxfmt.h>

#include <fstream>

#include "scope.hh"

namespace Slake {
	namespace Compiler {
		void deinit();

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

			inline LocalVar(std::uint32_t stackPos, std::shared_ptr<TypeName> type) : stackPos(stackPos), type(type) {}
			inline LocalVar() {}
		};

		struct Ins {
			Opcode opcode;
			std::vector<std::shared_ptr<Expr>> operands;

			inline Ins(Opcode opcode, std::vector<std::shared_ptr<Expr>> operands) : opcode(opcode), operands(operands) {}
		};

		struct Fn {
			std::vector<Ins> body;
			std::unordered_map<std::string, std::uint32_t> labels;
			std::unordered_map<std::string, LocalVar> lvars;

			inline void insertLabel(std::string name) { labels[name] = body.size(); };
			inline std::uint32_t getLabel(std::string name) { return labels.at(name); }

			inline void insertIns(const Ins& ins) {
				body.push_back(ins);
			}
			template <typename... Args>
			inline void insertIns(const Ins& ins, Args... args) {
				insertIns(ins);
				insertIns(args...);
			}
		};

		struct State {
			std::unordered_map<std::string, std::shared_ptr<Fn>> fnDefs;
			std::shared_ptr<Scope> scope;
			std::uint32_t stackCur = 0, nContinueLevel = 0, nBreakLevel = 0;
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

			inline State(std::shared_ptr<Scope> scope = std::shared_ptr<Scope>()) : scope(scope) {}
		};

		std::shared_ptr<TypeName> evalExprType(std::shared_ptr<State> s, std::shared_ptr<Expr> expr, bool isRecursing = false);
		std::shared_ptr<Expr> evalConstExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s);

		void writeIns(std::shared_ptr<State> s, Opcode opcode, std::fstream& fs, std::initializer_list<std::shared_ptr<Expr>> operands = {});
		void compileRightExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing = false);
		void compileLeftExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing = false);
		void compileStmt(std::shared_ptr<Compiler::Stmt> src, std::shared_ptr<State> s);
		void compile(std::shared_ptr<Scope> scope, std::fstream& fs, bool isTopLevel = true);
	}
}

#endif
