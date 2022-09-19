///
/// @file lex.h
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Lexical analyzer definitions
/// @version 0.1
/// @date 2022-09-18
///
/// @copyright Copyright (c) 2022 Vizard Contributors
///
#ifndef _VZC_SYNTAX_LEX_H_
#define _VZC_SYNTAX_LEX_H_

#include <blare/lexer.hpp>

namespace Vzc {
	namespace Syntax {
		enum Tokens {
			TOK_INT_L = SCHAR_MAX,	// Integer literal
			TOK_UINT_L,				// Unsigned integer literal
			TOK_LONG_L,				// Long integer literal
			TOK_ULONG_L,			// Unsigned long integer literal
			TOK_FLOAT_L,			// Floating-point literal
			TOK_DOUBLE_L,			// Double precision Floating-point literal
			TOK_STRING_L,			// String literal

			TOK_ID,		   // Identifier
			TOK_TYPENAME,  // Type name

			OP_SCOPE,	   // ::
			OP_INLINE_SW,  // =>
			OP_LAND,	   // &&
			OP_LOR,		   // ||
			OP_LSHIFT,	   // <<
			OP_RSHIFT,	   // >>
			OP_ASSIGNOP,   // A single operator with assignment

			KW_CONST,	  // const
			KW_FINAL,	  // final
			KW_PUB,		  // pub
			KW_IF,		  // if
			KW_ELIF,	  // elif
			KW_ELSE,	  // else
			KW_SWITCH,	  // switch
			KW_CASE,	  // case
			KW_DEFAULT,	  // default
			KW_RETURN,	  // return
			KW_FOR,		  // for
			KW_WHILE,	  // wile
			KW_TIMES,	  // times
			KW_BREAK,	  // break
			KW_CONTINUE,  // continue
			KW_ASYNC,	  // async
			KW_AWAIT,	  // await
			KW_FN,		  // fn
			KW_CLASS,	  // class
			KW_ENUM,	  // enum
			KW_IMPORT,	  // import
			KW_NULL, // null
		};

		using IdSym = Blare::ValSym<std::string>;

		using StringLiteralSym = Blare::ValSym<std::string>;
		using IntLiteralSym = Blare::ValSym<std::int32_t>;
		using UIntLiteralSym = Blare::ValSym<std::uint32_t>;
		using LongLiteralSym = Blare::ValSym<std::int64_t>;
		using ULongLiteralSym = Blare::ValSym<std::uint64_t>;
		using FloatLiteralSym = Blare::ValSym<float>;
		using DoubleLiteralSym = Blare::ValSym<double>;

		using TypenameSym = Blare::ValSym<std::string>;

		class VzcLexer : public Blare::Lexer {
		public:
			VzcLexer();
			virtual inline ~VzcLexer() {}
		};
	}
}

#endif
