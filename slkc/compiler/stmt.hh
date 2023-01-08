#ifndef _SLKC_COMPILER_STMT_HH
#define _SLKC_COMPILER_STMT_HH

#include "typename.hh"

namespace Slake {
	namespace Compiler {
		enum class StmtType : int {
			NONE = 0,
			EXPR,
			RETURN,
			IF,
			SWITCH,
			FOR,
			WHILE,
			TIMES,
			CONTINUE,
			BREAK,
			CODEBLOCK,
			VAR_DEF
		};

		class Stmt :public IStringifiable {
		public:
			virtual inline ~Stmt() {}
			virtual inline StmtType getType() { return StmtType::NONE; }

			virtual inline std::string toString() const override {
				return "(Statement);";
			}
		};

		class ExprStmt : public Stmt {
		public:
			std::shared_ptr<Expr> expr;
			std::shared_ptr<ExprStmt> next;

			inline ExprStmt(std::shared_ptr<Expr> expr, std::shared_ptr<ExprStmt> next = std::shared_ptr<ExprStmt>()) {
				this->expr = expr;
				this->next = next;
			}
			virtual inline ~ExprStmt() {}
			virtual inline StmtType getType() override { return StmtType::EXPR; }
		};

		class ReturnStmt : public Stmt {
		public:
			std::shared_ptr<Expr> expr;

			inline ReturnStmt(std::shared_ptr<Expr> expr = std::shared_ptr<Expr>()) {
				this->expr = expr;
			}
			virtual inline ~ReturnStmt() {}
			virtual inline StmtType getType() override { return StmtType::RETURN; }
		};

		class IfStmt : public Stmt {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Stmt> thenBlock, elseBlock;

			inline IfStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> thenBlock, std::shared_ptr<Stmt> elseBlock) {
				this->condition = condition;
				this->thenBlock = thenBlock;
				this->elseBlock = elseBlock;
			}
			virtual inline ~IfStmt() {}
			virtual inline StmtType getType() override { return StmtType::RETURN; }
		};

		class WhileStmt : public Stmt {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Stmt> execBlock;

			inline WhileStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> execBlock) {
				this->condition = condition;
				this->execBlock = execBlock;
			}
			virtual inline ~WhileStmt() {}
			virtual inline StmtType getType() override { return StmtType::WHILE; }
		};

		class ContinueStmt : public Stmt {
		public:
			inline ContinueStmt() {}
			virtual inline ~ContinueStmt() {}
			virtual inline StmtType getType() override { return StmtType::CONTINUE; }
		};

		class BreakStmt : public Stmt {
		public:
			inline BreakStmt() {}
			virtual inline StmtType getType() override { return StmtType::BREAK; }
		};

		class CodeBlock : public Stmt {
		public:
			std::vector<std::shared_ptr<Stmt>> ins;

			inline CodeBlock() {}
			virtual inline ~CodeBlock() {}
			virtual inline StmtType getType() override { return StmtType::CODEBLOCK; }

			virtual inline std::string toString() const override {
				std::string s = "{\n";
				for (auto i : ins) {
					s += std::to_string(*i) + "\n";
				}
				return s;
			}
		};

		class VarDecl final {
		public:
			std::string name;
			std::shared_ptr<Expr> initValue;

			VarDecl(std::string name, std::shared_ptr<Expr> initValue = std::shared_ptr<Expr>()) {
				this->name = name;
				this->initValue = initValue;
			}
			virtual ~VarDecl() {}
		};

		struct VarDeclList final {
			std::vector<std::shared_ptr<VarDecl>> decls;

			std::shared_ptr<VarDecl> operator[](std::string name) {
				for (auto &i : decls) {
					if (i->name == name)
						return i;
				}
				return std::shared_ptr<VarDecl>();
			}
		};

		class VarDefStmt : public Stmt, public IAccessModified {
		public:
			std::shared_ptr<TypeName> typeName;
			std::shared_ptr<VarDeclList> declList;

			inline VarDefStmt(AccessModifier accessModifier, std::shared_ptr<TypeName> typeName, std::shared_ptr<VarDeclList> declList) {
				this->accessModifier = accessModifier;
				this->typeName = typeName;
				this->declList = declList;
			}
			virtual inline ~VarDefStmt() {}
			virtual inline StmtType getType() override { return StmtType::VAR_DEF; }
		};

		class ForStmt : public Stmt {
		public:
			std::shared_ptr<VarDefStmt> varDecl;
			std::shared_ptr<Expr> condition;
			std::shared_ptr<ExprStmt> endExpr;
			std::shared_ptr<Stmt> execBlock;

			inline ForStmt(std::shared_ptr<VarDefStmt> varDecl, std::shared_ptr<Expr> condition, std::shared_ptr<ExprStmt> endExpr, std::shared_ptr<Stmt> execBlock) {
				this->varDecl = varDecl;
				this->condition = condition;
				this->endExpr = endExpr;
				this->execBlock = execBlock;
			}
			virtual inline ~ForStmt() {}
			virtual inline StmtType getType() override { return StmtType::FOR; }
		};

		class TimesStmt : public Stmt {
		public:
			std::shared_ptr<Expr> timesExpr;
			std::shared_ptr<Stmt> execBlock;

			inline TimesStmt(std::shared_ptr<Expr> timesExpr, std::shared_ptr<Stmt> execBlock) {
				this->timesExpr = timesExpr;
				this->execBlock = execBlock;
			}
			virtual inline ~TimesStmt() {}
			virtual inline StmtType getType() override { return StmtType::TIMES; }
		};

		class SwitchCase final {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Stmt> x;

			inline SwitchCase(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> x) {
				this->condition = condition;
				this->x = x;
			}
			virtual inline ~SwitchCase() {}
		};

		using SwitchCaseList = std::vector<std::shared_ptr<SwitchCase>>;

		class SwitchStmt : public Stmt {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<SwitchCaseList> caseList;

			inline SwitchStmt(std::shared_ptr<Expr> condition, std::shared_ptr<SwitchCaseList> caseList) {
				this->condition = condition;
				this->caseList = caseList;
			}
			virtual inline ~SwitchStmt() {}
			virtual inline StmtType getType() override { return StmtType::SWITCH; }
		};
	}
}

#endif
