#ifndef _SLKC_COMPILER_STMT_HH
#define _SLKC_COMPILER_STMT_HH

#include "expr.hh"

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

		class Stmt : public IStringifiable {
		public:
			virtual inline ~Stmt() {}
			virtual inline StmtType getType() { return StmtType::NONE; }

			virtual inline std::string toString() const override {
				return genIndentStr() + "(Statement);";
			}
		};

		class ExprStmt : public Stmt, public ILocated {
		public:
			std::shared_ptr<Expr> expr;
			std::shared_ptr<ExprStmt> next;

			inline ExprStmt(std::shared_ptr<Expr> expr, std::shared_ptr<ExprStmt> next = std::shared_ptr<ExprStmt>()) {
				this->expr = expr;
				this->next = next;
			}
			virtual inline ~ExprStmt() {}
			virtual inline StmtType getType() override { return StmtType::EXPR; }

			virtual inline std::string toString() const override {
				return std::to_string(*expr) + (next ? ", " + std::to_string(*next) : ";");
			}

			virtual inline location getLocation() const override {
				return expr->getLocation();
			}
		};

		class ReturnStmt : public Stmt, public BasicLocated {
		public:
			std::shared_ptr<Expr> expr;

			inline ReturnStmt(location loc, std::shared_ptr<Expr> expr = std::shared_ptr<Expr>()) : BasicLocated(loc) {
				this->expr = expr;
			}
			virtual inline ~ReturnStmt() {}
			virtual inline StmtType getType() override { return StmtType::RETURN; }

			virtual inline std::string toString() const override {
				return "return" + (expr ? std::to_string(*expr) : "") + ";";
			}
		};

		class IfStmt : public Stmt, public BasicLocated {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Stmt> thenBlock, elseBlock;

			inline IfStmt(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> thenBlock, std::shared_ptr<Stmt> elseBlock) : BasicLocated(loc) {
				this->condition = condition;
				this->thenBlock = thenBlock;
				this->elseBlock = elseBlock;
			}
			virtual inline ~IfStmt() {}
			virtual inline StmtType getType() override { return StmtType::RETURN; }

			virtual inline std::string toString() const override {
				return "if (" + std::to_string(*condition) + ')' + std::to_string(*thenBlock) + (elseBlock ? " else " + std::to_string(*elseBlock) : "");
			}
		};

		class WhileStmt : public Stmt, public BasicLocated {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Stmt> execBlock;

			inline WhileStmt(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> execBlock) : BasicLocated(loc) {
				this->condition = condition;
				this->execBlock = execBlock;
			}
			virtual inline ~WhileStmt() {}
			virtual inline StmtType getType() override { return StmtType::WHILE; }

			virtual inline std::string toString() const override {
				return "while (" + std::to_string(*condition) + ") " + std::to_string(*execBlock);
			}
		};

		class ContinueStmt : public Stmt, public BasicLocated {
		public:
			inline ContinueStmt(location loc) : BasicLocated(loc) {}
			virtual inline ~ContinueStmt() {}
			virtual inline StmtType getType() override { return StmtType::CONTINUE; }
		};

		class BreakStmt : public Stmt, public BasicLocated {
		public:
			inline BreakStmt(location loc) : BasicLocated(loc) {}
			virtual inline StmtType getType() override { return StmtType::BREAK; }
		};

		class CodeBlock : public Stmt, public BasicLocated {
		public:
			std::vector<std::shared_ptr<Stmt>> ins;

			inline CodeBlock(location loc) : BasicLocated(loc) {}
			virtual inline ~CodeBlock() {}
			virtual inline StmtType getType() override { return StmtType::CODEBLOCK; }

			virtual inline std::string toString() const override {
				std::string s = "{\n";
				indentLevel++;
				for (auto& i : ins) {
					s += genIndentStr() + std::to_string(*i) + "\n";
				}
				indentLevel--;
				s += genIndentStr() + "}";
				return s;
			}
		};

		class VarDecl final : public IStringifiable,
							  public BasicLocated {
		public:
			std::string name;
			std::shared_ptr<Expr> initValue;

			VarDecl(location loc, std::string name, std::shared_ptr<Expr> initValue = std::shared_ptr<Expr>()) : BasicLocated(loc) {
				this->name = name;
				this->initValue = initValue;
			}
			virtual ~VarDecl() {}

			virtual inline std::string toString() const override {
				return name + (initValue ? " = " + std::to_string(*initValue) : "");
			}
		};

		class VarDeclList final : public IStringifiable,
								  public std::vector<std::shared_ptr<VarDecl>> {
		public:
			virtual inline ~VarDeclList() {}

			using vector::operator[];
			inline std::shared_ptr<VarDecl> operator[](std::string name) const {
				for (auto& i : (*this)) {
					if (i->name == name)
						return i;
				}
				return std::shared_ptr<VarDecl>();
			}

			virtual inline std::string toString() const override {
				std::string s;

				auto i = this->begin();
				if (i != this->end())
					s = std::to_string((**i++));
				while (i != this->end())
					s += ", " + std::to_string((**i++));

				return s;
			}
		};

		class VarDefStmt : public Stmt, public IAccessModified, public ILocated {
		public:
			std::shared_ptr<TypeName> typeName;
			VarDeclList declList;
			bool isNative;

			inline VarDefStmt(AccessModifier accessModifier, std::shared_ptr<TypeName> typeName, VarDeclList declList, bool isNative = false) : IAccessModified(accessModifier) {
				this->typeName = typeName;
				this->declList = declList;
				this->isNative = isNative;
			}
			virtual inline ~VarDefStmt() {}
			virtual inline StmtType getType() override { return StmtType::VAR_DEF; }

			virtual inline std::string toString() const override {
				return std::to_string(*typeName) + " " + std::to_string(declList) + ";";
			}

			virtual inline location getLocation() const override {
				return typeName->getLocation();
			}
		};

		class ForStmt : public Stmt,
						public BasicLocated {
		public:
			std::shared_ptr<VarDefStmt> varDecl;
			std::shared_ptr<Expr> condition;
			std::shared_ptr<ExprStmt> endExpr;
			std::shared_ptr<Stmt> execBlock;

			inline ForStmt(location loc, std::shared_ptr<VarDefStmt> varDecl, std::shared_ptr<Expr> condition, std::shared_ptr<ExprStmt> endExpr, std::shared_ptr<Stmt> execBlock) : BasicLocated(loc) {
				this->varDecl = varDecl;
				this->condition = condition;
				this->endExpr = endExpr;
				this->execBlock = execBlock;
			}
			virtual inline ~ForStmt() {}
			virtual inline StmtType getType() override { return StmtType::FOR; }

			virtual inline std::string toString() const override {
				std::string endExprStr = std::to_string(*endExpr);
				endExprStr.pop_back();
				return "for (" + std::to_string(*varDecl) + std::to_string(*condition) + ";" + endExprStr + ")" + std::to_string(*execBlock);
			}
		};

		class TimesStmt : public Stmt,
						  public BasicLocated {
		public:
			std::shared_ptr<Expr> timesExpr;
			std::shared_ptr<Stmt> execBlock;

			inline TimesStmt(location loc, std::shared_ptr<Expr> timesExpr, std::shared_ptr<Stmt> execBlock) : BasicLocated(loc) {
				this->timesExpr = timesExpr;
				this->execBlock = execBlock;
			}
			virtual inline ~TimesStmt() {}
			virtual inline StmtType getType() override { return StmtType::TIMES; }

			virtual inline std::string toString() const override {
				return "times (" + std::to_string(*timesExpr) + ')' + std::to_string(*execBlock);
			}
		};

		class SwitchCase final : public IStringifiable,
								 public BasicLocated {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Stmt> x;

			inline SwitchCase(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> x) : BasicLocated(loc) {
				this->condition = condition;
				this->x = x;
			}
			virtual inline ~SwitchCase() {}

			virtual inline std::string toString() const override {
				return "case " + std::to_string(*condition) + ": " + std::to_string(*x);
			}
		};

		using SwitchCaseList = std::vector<std::shared_ptr<SwitchCase>>;

		class SwitchStmt : public Stmt,
						   public BasicLocated {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<SwitchCaseList> caseList;

			inline SwitchStmt(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<SwitchCaseList> caseList) : BasicLocated(loc) {
				this->condition = condition;
				this->caseList = caseList;
			}
			virtual inline ~SwitchStmt() {}
			virtual inline StmtType getType() override { return StmtType::SWITCH; }
		};
	}
}

#endif
