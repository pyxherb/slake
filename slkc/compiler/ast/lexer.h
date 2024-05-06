#ifndef _SLKC_COMPILER_SYNTAX_LEXER_H_
#define _SLKC_COMPILER_SYNTAX_LEXER_H_

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <stdexcept>

#include "astnode.h"
#include "ref.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		enum class TokenId : int {
			End = -1,

			Unknown,

			Comma,
			Question,
			Colon,
			Semicolon,
			LBracket,
			RBracket,
			LBrace,
			RBrace,
			LParenthese,
			RParenthese,
			At,
			Dot,
			VarArg,

			ScopeOp,
			WrapOp,
			MatchOp,
			LAndOp,
			LOrOp,
			IncOp,
			DecOp,
			AddOp,
			SubOp,
			MulOp,
			DivOp,
			ModOp,
			AndOp,
			OrOp,
			XorOp,
			LNotOp,
			NotOp,
			AssignOp,
			AddAssignOp,
			SubAssignOp,
			MulAssignOp,
			DivAssignOp,
			ModAssignOp,
			AndAssignOp,
			OrAssignOp,
			XorAssignOp,
			NotAssignOp,
			LshAssignOp,
			RshAssignOp,
			StrictEqOp,
			StrictNeqOp,
			EqOp,
			NeqOp,
			LshOp,
			RshOp,
			LtEqOp,
			GtEqOp,
			LtOp,
			GtOp,
			DollarOp,

			AsKeyword,
			AsyncKeyword,
			AwaitKeyword,
			BaseKeyword,
			BreakKeyword,
			CaseKeyword,
			CatchKeyword,
			ClassKeyword,
			ConstKeyword,
			ContinueKeyword,
			DeleteKeyword,
			DefaultKeyword,
			ElseKeyword,
			EnumKeyword,
			FalseKeyword,
			FnKeyword,
			ForKeyword,
			FinalKeyword,
			IfKeyword,
			ImportKeyword,
			LetKeyword,
			ModuleKeyword,
			NativeKeyword,
			NewKeyword,
			NullKeyword,
			OverrideKeyword,
			OperatorKeyword,
			PubKeyword,
			ReturnKeyword,
			StaticKeyword,
			StructKeyword,
			SwitchKeyword,
			ThisKeyword,
			ThrowKeyword,
			TraitKeyword,
			TypeofKeyword,
			InterfaceKeyword,
			TrueKeyword,
			TryKeyword,
			UseKeyword,
			WhileKeyword,
			YieldKeyword,

			I8TypeName,
			I16TypeName,
			I32TypeName,
			I64TypeName,
			U8TypeName,
			U16TypeName,
			U32TypeName,
			U64TypeName,
			F32TypeName,
			F64TypeName,
			StringTypeName,
			BoolTypeName,
			AutoTypeName,
			VoidTypeName,
			AnyTypeName,

			IntLiteral,
			LongLiteral,
			UIntLiteral,
			ULongLiteral,
			F32Literal,
			F64Literal,
			StringLiteral,
			RawStringLiteral,

			Id,

			Whitespace,
			NewLine,
			Comment
		};

		const char *getTokenName(TokenId tokenId);

		struct TokenExtension {
			virtual ~TokenExtension() = default;
		};

		template <typename T>
		struct LiteralTokenExtension : public TokenExtension {
			T data;

			inline LiteralTokenExtension(T data) : data(data) {}
			virtual ~LiteralTokenExtension() = default;
		};

		using IntLiteralTokenExtension = LiteralTokenExtension<int32_t>;
		using LongLiteralTokenExtension = LiteralTokenExtension<int64_t>;
		using UIntLiteralTokenExtension = LiteralTokenExtension<uint32_t>;
		using ULongLiteralTokenExtension = LiteralTokenExtension<uint64_t>;
		using F32LiteralTokenExtension = LiteralTokenExtension<float>;
		using F64LiteralTokenExtension = LiteralTokenExtension<double>;
		using IntLiteralTokenExtension = LiteralTokenExtension<int32_t>;
		using StringLiteralTokenExtension = LiteralTokenExtension<std::string>;

		struct MajorContext;

		struct Token {
			TokenId tokenId;
			Location beginLocation, endLocation;
			std::string text;
			std::unique_ptr<TokenExtension> exData;
		};

		class LexicalError : public std::runtime_error {
		public:
			Location location;

			inline LexicalError(std::string_view s, Location location) : runtime_error(s.data()), location(location) {}
			virtual ~LexicalError() = default;
		};

		struct LexerContext {
			size_t prevIndex = 0;
			size_t curIndex = 0;
		};

		class Lexer {
		private:
			Token _endToken;

		public:
			LexerContext context;

			std::vector<Token> tokens;

			void lex(std::string_view src);

			const Token &nextToken(bool keepNewLine = false, bool keepWhitespace = false, bool keepComment = false);
			const Token &peekToken(bool keepNewLine = false, bool keepWhitespace = false, bool keepComment = false);

			inline void reset() {
				context = {};
				_endToken = {};
				tokens.clear();
			}

			inline size_t getTokenByLocation(Location location) {
				for (size_t i = 0; i < tokens.size(); ++i) {
					if (tokens[i].beginLocation <= location && tokens[i].endLocation >= location)
						return i;
				}

				return SIZE_MAX;
			}

			inline size_t getTokenIndex(const Token &token) {
				return (&token) - tokens.data();
			}
		};
	}
}

#endif
