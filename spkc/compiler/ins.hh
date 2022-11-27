#ifndef _SPKC_SYNTAX_INS_HH
#define _SPKC_SYNTAX_INS_HH

#include "typename.hh"

namespace SpkC {
	namespace Syntax {
		enum class InstructionType : int {
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

		class Instruction : public IToken {
		public:
			virtual inline ~Instruction() {}
			virtual inline InstructionType getType() { return InstructionType::NONE; }
		};

		class ExprInstruction : public Instruction {
		public:
			std::shared_ptr<Expr> expr;
			std::shared_ptr<Instruction> next;

			inline ExprInstruction(std::shared_ptr<Expr> expr, std::shared_ptr<Instruction> next = std::shared_ptr<Instruction>()) {
				this->expr = expr;
				this->next = next;
			}
			virtual inline ~ExprInstruction() {
			}
			virtual inline InstructionType getType() override { return InstructionType::EXPR; }
		};

		class ReturnInstruction : public Instruction {
		public:
			std::shared_ptr<Expr> expr;

			inline ReturnInstruction(std::shared_ptr<Expr> expr) {
				this->expr = expr;
			}
			virtual inline ~ReturnInstruction() {
			}
			virtual inline InstructionType getType() override { return InstructionType::RETURN; }
		};

		class IfInstruction : public Instruction {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Instruction> thenBlock, elseBlock;

			inline IfInstruction(std::shared_ptr<Expr> condition, std::shared_ptr<Instruction> thenBlock, std::shared_ptr<Instruction> elseBlock) {
				this->condition = condition;
				this->thenBlock = thenBlock;
				this->elseBlock = elseBlock;
			}
			virtual inline ~IfInstruction() {
			}
			virtual inline InstructionType getType() override { return InstructionType::RETURN; }
		};

		class WhileInstruction : public Instruction {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<Instruction> execBlock;

			inline WhileInstruction(std::shared_ptr<Expr> condition, std::shared_ptr<Instruction> execBlock) {
				this->condition = condition;
				this->execBlock = execBlock;
			}
			virtual inline ~WhileInstruction() {
			}
			virtual inline InstructionType getType() override { return InstructionType::WHILE; }
		};

		class ContinueInstruction : public Instruction {
		public:
			inline ContinueInstruction() {}
			virtual inline InstructionType getType() override { return InstructionType::CONTINUE; }
		};

		class BreakInstruction : public Instruction {
		public:
			inline BreakInstruction() {}
			virtual inline InstructionType getType() override { return InstructionType::BREAK; }
		};

		class CodeBlock : public Instruction {
		public:
			std::vector<std::shared_ptr<Instruction>> ins;

			inline CodeBlock() {}
			virtual inline ~CodeBlock() {
			}

			virtual inline InstructionType getType() override { return InstructionType::BREAK; }
		};

		class VarDecl final : public IToken {
		public:
			std::string name;
			std::shared_ptr<Expr> initValue;

			VarDecl(std::string name, std::shared_ptr<Expr> initValue = std::shared_ptr<Expr>()) {
				this->name = name;
				this->initValue = initValue;
			}
			~VarDecl() {
			}
		};

		struct VarDeclList final : public IToken {
			std::vector<std::shared_ptr<VarDecl>> decls;

			std::shared_ptr<VarDecl> operator[](std::string name) {
				for (auto &i : decls) {
					if (i->name == name)
						return i;
				}
				return std::shared_ptr<VarDecl>();
			}
		};

		class VarDefInstruction : public Instruction {
		public:
			AccessModifier accessModifier;
			std::shared_ptr<TypeName> typeName;
			std::shared_ptr<VarDeclList> declList;

			inline VarDefInstruction(AccessModifier accessModifier, std::shared_ptr<TypeName> typeName, std::shared_ptr<VarDeclList> declList) {
				this->accessModifier = accessModifier;
				this->typeName = typeName;
				this->declList = declList;
			}
			virtual inline ~VarDefInstruction() {
			}
			virtual inline InstructionType getType() override { return InstructionType::VAR_DEF; }
		};

		class ForInstruction : public Instruction {
		public:
			std::shared_ptr<VarDeclList> varDecl;
			std::shared_ptr<Expr> condition, endExpr;
			std::shared_ptr<Instruction> execBlock;

			inline ForInstruction(std::shared_ptr<VarDeclList> varDecl, std::shared_ptr<Expr> condition, std::shared_ptr<Expr> endExpr, std::shared_ptr<Instruction> execBlock) {
				this->varDecl = varDecl;
				this->condition = condition;
				this->endExpr = endExpr;
				this->execBlock = execBlock;
			}
			virtual inline ~ForInstruction() {
			}
			virtual inline InstructionType getType() override { return InstructionType::FOR; }
		};
	}
}

#endif
