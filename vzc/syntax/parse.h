#ifndef SLAKE_SYNTAX_PARSE_H_
#define SLAKE_SYNTAX_PARSE_H_

#include <map>
#include <set>

#include "lex.h"

namespace Vzc {
	namespace Syntax {
		using namespace Blare;

		class SyntaxError final : public std::runtime_error {
		public:
			std::shared_ptr<Sym> sym;

			explicit inline SyntaxError(std::shared_ptr<Sym> sym)
				: runtime_error("Syntax error") {
				this->sym = sym;
			}
			explicit inline SyntaxError(std::shared_ptr<Sym> sym, std::string msg)
				: runtime_error(msg) {
				this->sym = sym;
			}
			virtual inline ~SyntaxError() {}
		};

		class VzcParser {
		protected:
			class Scope;
			class Expr;
			class Function;

			class Expr {
			public:
				enum class Type : std::uint8_t {
					LITERAL = 0,  // Literal Value (Compile-Time Constant)
					IDENTIFIER,	  // Identifier
					UNARY_OP,	  // Unary Operation
					BINARY_OP,	  // Binary Operation
					TERNARY_OP,	  // Ternary Operation
					INLINE_SW,	  // Inline Switch
				};

				enum class UnaryOp : std::uint8_t {
					NEG = 0,  // Negate
					NOT,	  // Bitwise NOT
					LNOT,	  // Logic NOT
					INC_F,	  // Increase Forward
					DEC_F,	  // Decrease Forward
					INC_B,	  // Increase Backward
					DEC_B,	  // Decrease Backward
				};

				enum class BinaryOp : std::uint8_t {
					ADD = 0,  // Add
					SUB,	  // Subtract
					MUL,	  // Multiply
					DIV,	  // Divide
					MOD,	  // Modulo
					AND,	  // Bitwise AND
					OR,		  // Bitwise OR
					XOR,	  // Bitwise XOR
					LAND,	  // Logic AND
					LOR,	  // Logic OR
					ASSIGN,	  // Assign
					LT,		  // Less than
					GT,		  // Greater than
					EQ,		  // Equal to
					LTEQ,	  // Less or equal
					GTEQ,	  // Greater or equal
				};

				Expr& operator=(Expr&) = delete;
				Expr& operator=(Expr&&) = delete;
			};

			class Function {
				constexpr static std::uint8_t FL_PUB = 0x01,
											  FL_DEPRECATED = 0x02;

				std::uint8_t flags;
				std::list<Expr> body;
			};

			class Scope {
				std::map<std::string, Function> functions;
			};

			/**
			 * @brief Fetch a symbol and auto-increase the iterator.
			 *
			 * @param i Source iterator.
			 * @return std::shared_ptr<Sym> Fetched symbol.
			 */
			inline std::shared_ptr<Sym> _nextSym(SymList::iterator& i) {
				return *(i++);
			}
			/**
			 * @brief Fetch a symbol with error detection and auto-increase the
			 * iterator.
			 *
			 * @param i Source iterator.
			 * @param expectedTokens Expected tokens.
			 * @return std::shared_ptr<Sym> Fetched symbols.
			 */
			inline std::shared_ptr<Sym> _nextSym(SymList::iterator& i,
				std::set<Token> expectedTokens,
				std::string errmsg = "unexpected token") {
				if (!expectedTokens.count((*i)->token))
					throw SyntaxError(*i, errmsg);
				return *(i++);
			}

			inline std::shared_ptr<Sym> _nextSym(SymList::iterator& i,
				Token expectedToken,
				std::string errmsg = "unexpected token") {
				if ((*i)->token != expectedToken) throw SyntaxError(*i, errmsg);
				return *(i++);
			}

		public:
			inline VzcParser() {}
			inline ~VzcParser() {}

			std::shared_ptr<Expr> parseExpr(SymList::iterator& i);
			void parseAttrib(SymList::iterator& i);
			void parseImport(SymList::iterator& i);
			void parse(SymList& syms);
		};
	}
}

#endif
