#ifndef _SLKC_COMPILER_STATE_HH
#define _SLKC_COMPILER_STATE_HH

#include <slake/opcode.h>

#include "../ast/scope.hh"

namespace Slake {
	namespace Compiler {
		struct LocalVar {
			uint32_t stackPos;
			std::shared_ptr<TypeName> type;
			bool isParam = false;

			inline LocalVar &operator=(const LocalVar &x) {
				return *this = std::move(x);
			}

			inline LocalVar &operator=(const LocalVar &&x) {
				stackPos = x.stackPos;
				type = x.type;
				isParam = x.isParam;
				return *this;
			}

			inline LocalVar(uint32_t stackPos, std::shared_ptr<TypeName> type, bool isParam = false) : stackPos(stackPos), type(type), isParam(isParam) {}
			inline LocalVar() {}
			inline LocalVar(const LocalVar &x) { *this = x; }
			inline LocalVar(const LocalVar &&x) { *this = x; }
		};

		struct Ins {
			Opcode opcode;
			std::vector<std::shared_ptr<Expr>> operands;

			inline Ins(Opcode opcode, std::vector<std::shared_ptr<Expr>> operands) : opcode(opcode), operands(operands) {}
		};

		struct Fn {
			std::vector<Ins> body;
			std::unordered_map<std::string, uint32_t> labels;

			inline void insertLabel(std::string name) { labels[name] = (uint32_t)body.size(); };
			inline uint32_t getLabel(std::string name) { return labels.at(name); }

			inline void insertIns(const Ins &ins) {
				body.push_back(ins);
			}
			template <typename... Args>
			inline void insertIns(const Ins &ins, Args... args) {
				insertIns(ins);
				insertIns(args...);
			}
		};

		struct Context {
			std::unordered_map<std::string, LocalVar> lvars;
			uint32_t stackCur = 0, nContinueLevel = 0, nBreakLevel = 0;
			bool returned = false;

			inline Context() {}
			inline Context(const Context &context) { *this = context; }
			inline Context(const Context &&context) { *this = context; }

			inline Context &operator=(const Context &x) {
				lvars = x.lvars;
				stackCur = x.stackCur, nContinueLevel = x.nContinueLevel, nBreakLevel = x.nBreakLevel;
				returned = x.returned;
				return *this;
			}
			inline Context &operator=(const Context &&x) {
				lvars = x.lvars;
				stackCur = x.stackCur, nContinueLevel = x.nContinueLevel, nBreakLevel = x.nBreakLevel;
				returned = x.returned;
				return *this;
			}
		};

		enum class OptimizeLevel {

		};

		struct State {
			std::unordered_map<std::string, std::shared_ptr<Fn>> fnDefs;
			std::unordered_map<std::string, std::shared_ptr<RefExpr>> imports;
			std::shared_ptr<Scope> scope;
			Context context;
			std::string currentFn;

			std::shared_ptr<TypeName> desiredType;
			std::shared_ptr<Expr> calleeParent;
			bool isLastResolvedRefVar = false;// Is last resolved 

			inline State(std::shared_ptr<Scope> scope = std::shared_ptr<Scope>()) : scope(scope) {}

			void writeValue(std::shared_ptr<Expr> src, std::fstream &fs);
			void writeIns(Opcode opcode, std::fstream &fs, std::initializer_list<std::shared_ptr<Expr>> operands = {});

			inline void enterLoop() noexcept {
				context.nContinueLevel++, context.nBreakLevel++;
			}
			inline void leaveLoop() noexcept {
				context.nContinueLevel--, context.nBreakLevel--;
			}

			inline void enterSwitch() noexcept { context.nContinueLevel++; }
			inline void leaveSwitch() noexcept { context.nContinueLevel--; }

			void compileTypeName(std::fstream &fs, std::shared_ptr<TypeName> j);
			void compileRightExpr(std::shared_ptr<Expr> expr, bool isRecursing = false);
			void compileLeftExpr(std::shared_ptr<Expr> expr, bool isRecursing = false);
			void compileStmt(std::shared_ptr<Compiler::Stmt> src);
			void compileArg(std::shared_ptr<Expr> expr);
			void compile(std::shared_ptr<Scope> scope, std::fstream &fs, bool isTopLevel = true);

			std::shared_ptr<TypeName> evalExprType(std::shared_ptr<Expr> expr, bool isRecursing = false);
			std::shared_ptr<Expr> evalConstExpr(std::shared_ptr<Expr> expr);

			inline void clearTempAttribs() {
				desiredType.reset();
				calleeParent.reset();
				isLastResolvedRefVar = false;
			}
		};
	}
}

#endif
