#ifndef _SLKC_PARSER_H_
#define _SLKC_PARSER_H_

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <forward_list>

namespace Slake {
	namespace Compiler {
		enum Token : std::uint16_t {
			T_ID = 0,
			T_VARARG,

			L_INT,
			L_UINT,
			L_LONG,
			L_ULONG,
			L_FLOAT,
			L_DOUBLE,
			L_STRING,

			KW_ASYNC,
			KW_AWAIT,
			KW_BASE,
			KW_BREAK,
			KW_CASE,
			KW_CATCH,
			KW_CLASS,
			KW_CONST,
			KW_CONTINUE,
			KW_DEFAULT,
			KW_DELETE,
			KW_ELSE,
			KW_ENUM,
			KW_FALSE,
			KW_FINAL,
			KW_FINALLY,
			KW_FN,
			KW_FOR,
			KW_IF,
			KW_NATIVE,
			KW_NEW,
			KW_NULL,
			KW_OVERRIDE,
			KW_OPERATOR,
			KW_PUB,
			KW_RETURN,
			KW_STATIC,
			KW_STRUCT,
			KW_SWITCH,
			KW_THIS,
			KW_THROW,
			KW_TIMES,
			KW_TRAIT,
			KW_TRUE,
			KW_TRY,
			KW_USE,
			KW_VAR,
			KW_WHILE,

			TN_I8,
			TN_I16,
			TN_I32,
			TN_I64,
			TN_ISIZE,
			TN_U8,
			TN_U16,
			TN_U32,
			TN_U64,
			TN_USIZE,
			TN_FLOAT,
			TN_DOUBLE,
			TN_STRING,
			TN_BOOL,
			TN_AUTO,
			TN_VOID,

			OP_ADD,
			OP_SUB,
			OP_MUL,
			OP_DIV,
			OP_MOD,
			OP_AND,
			OP_OR,
			OP_XOR,
			OP_REV,
			OP_NOT,
			OP_TERNARY,
			OP_ASSIGN,
			OP_ASSIGN_ADD,
			OP_ASSIGN_SUB,
			OP_ASSIGN_MUL,
			OP_ASSIGN_DIV,
			OP_ASSIGN_MOD,
			OP_ASSIGN_AND,
			OP_ASSIGN_OR,
			OP_ASSIGN_XOR,
			OP_ASSIGN_REV,
			OP_ASSIGN_LSH,
			OP_ASSIGN_RSH,
			OP_ASSIGN_ACCESS,
			OP_EQ,
			OP_NEQ,
			OP_STRICTEQ,
			OP_STRICTNEQ,
			OP_LT,
			OP_GT,
			OP_LTEQ,
			OP_GTEQ,
			OP_RSH,
			OP_LSH,
			OP_LAND,
			OP_LOR,
			OP_INC,
			OP_DEC,
			OP_MATCH,
			OP_WRAP,
			OP_SCOPE,
			OP_DOLLAR,
			OP_DIRECTIVE,

			T_AT,
			T_LPARENTHESE,
			T_RPARENTHESE,
			T_LBRACKET,
			T_RBRACKET,
			T_LBRACE,
			T_RBRACE,
			T_COMMA,
			T_COLON,
			T_SEMICOLON,
			T_DOT,

			END = UINT16_MAX
		};

		struct Term {
			std::forward_list<std::forward_list<Token>> tokens;
			bool optional;
			std::uint16_t minRep, maxRep;
		};

		struct GeneratedTable {
			struct Item {
				std::uint8_t priority;
				GeneratedTable* next;
			};
			std::unordered_multimap<Token, Item> items;
		};

		class Parser {
		protected:
			std::unordered_set<GeneratedTable> _tables;
			void allocLeftRecurseTerm(Token tt, Term& t) {
				std::vector<std::forward_list<Token>*> normalTerms, recursedTerms;
				for (auto i : t.tokens) {
					if (*i.begin() == tt)
						recursedTerms.push_back(&i);
					else
						normalTerms.push_back(&i);
				}

				for (auto i : recursedTerms) {
					i->pop_front();
					for (auto j:normalTerms)
						j->insert_after(j->before_begin(), i->begin(), i->end());
				}
			}

		public:
		};
	}
}

#endif
