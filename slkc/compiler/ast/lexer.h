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
			NotOp,
			RevOp,
			AssignOp,
			AddAssignOp,
			SubAssignOp,
			MulAssignOp,
			DivAssignOp,
			ModAssignOp,
			AndAssignOp,
			OrAssignOp,
			XorAssignOp,
			RevAssignOp,
			LshAssignOp,
			RshAssignOp,
			SwapOp,
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

			Id
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
			size_t curIndex = 0;

			inline bool operator>(const LexerContext &rhs) const {
				return curIndex > rhs.curIndex;
			}

			inline bool operator<(const LexerContext &rhs) const {
				return curIndex < rhs.curIndex;
			}
		};

		class Lexer {
		private:
			Token _endToken;

		public:
			LexerContext context;

			std::vector<Token> tokens;

			void lex(std::string_view src);

			inline const Token &nextToken() {
				if (context.curIndex >= tokens.size())
					return _endToken;

				return tokens.at(context.curIndex++);
			}

			inline const Token &peekToken() {
				if (context.curIndex >= tokens.size())
					return _endToken;

				return tokens.at(context.curIndex);
			}
		};
	}
}

#endif