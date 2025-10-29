#ifndef _SLKC_AST_LEXER_H_
#define _SLKC_AST_LEXER_H_

#include "basedefs.h"
#include <slake/runtime.h>
#include <peff/base/deallocable.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>
#include <peff/advutils/shared_ptr.h>
#include <optional>
#include <variant>

namespace slkc {
	struct SourcePosition {
		size_t line, column;

		SLAKE_FORCEINLINE SourcePosition() : line(SIZE_MAX), column(SIZE_MAX) {}
		SLAKE_FORCEINLINE SourcePosition(size_t line, size_t column) : line(line), column(column) {}

		SLAKE_FORCEINLINE bool operator<(const SourcePosition &loc) const {
			if (line < loc.line)
				return true;
			if (line > loc.line)
				return false;
			return column < loc.column;
		}

		SLAKE_FORCEINLINE bool operator>(const SourcePosition &loc) const {
			if (line > loc.line)
				return true;
			if (line < loc.line)
				return false;
			return column > loc.column;
		}

		SLAKE_FORCEINLINE bool operator==(const SourcePosition &loc) const {
			return (line == loc.line) && (column == loc.column);
		}

		SLAKE_FORCEINLINE bool operator>=(const SourcePosition &loc) const {
			return ((*this) == loc) || ((*this) > loc);
		}

		SLAKE_FORCEINLINE bool operator<=(const SourcePosition &loc) const {
			return ((*this) == loc) || ((*this) < loc);
		}
	};

	struct SourceLocation {
		SourcePosition beginPosition, endPosition;
	};

	class Lexer;

	enum class TokenId : int {
		End = 0,

		Unknown,

		Comma,
		Question,
		Colon,
		Semicolon,
		LBracket,
		RBracket,
		LDBracket,
		RDBracket,
		LBrace,
		RBrace,
		LParenthese,
		RParenthese,
		At,
		Dot,
		VarArg,

		ScopeOp,
		ReturnTypeOp,
		MatchOp,
		LAndOp,
		LOrOp,
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
		CmpOp,
		DollarOp,

		AbstractKeyword,
		AllocaKeyword,
		AttributeKeyword,
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
		DoKeyword,
		ElseKeyword,
		EnumKeyword,
		FalseKeyword,
		FnKeyword,
		ForKeyword,
		FinalKeyword,
		GotoKeyword,
		IfKeyword,
		ImportKeyword,
		InKeyword,
		LetKeyword,
		MacroKeyword,
		MatchKeyword,
		ModuleKeyword,
		NativeKeyword,
		NewKeyword,
		NullKeyword,
		OperatorKeyword,
		OutKeyword,
		OverrideKeyword,
		PublicKeyword,
		ReturnKeyword,
		StaticKeyword,
		StructKeyword,
		SwitchKeyword,
		ThisKeyword,
		ThrowKeyword,
		TypeofKeyword,
		InterfaceKeyword,
		TrueKeyword,
		TryKeyword,
		UsingKeyword,
		VarKeyword,
		VirtualKeyword,
		WhileKeyword,
		WithKeyword,
		YieldKeyword,

		I8TypeName,
		I16TypeName,
		I32TypeName,
		I64TypeName,
		ISizeTypeName,
		U8TypeName,
		U16TypeName,
		U32TypeName,
		U64TypeName,
		USizeTypeName,
		F32TypeName,
		F64TypeName,
		StringTypeName,
		BoolTypeName,
		AutoTypeName,
		VoidTypeName,
		ObjectTypeName,
		AnyTypeName,
		SIMDTypeName,

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
		LineComment,
		BlockComment,
		DocumentationComment,

		MaxToken
	};

	SLAKE_FORCEINLINE bool isValidToken(TokenId tokenId) {
		return (((int)tokenId) >= 0) && (((int)tokenId) < (int)TokenId::MaxToken);
	}

	class TokenExtension {
	public:
		SLKC_API virtual ~TokenExtension();

		virtual void dealloc() = 0;
	};

	class IntTokenExtension : public TokenExtension {
	public:
		int data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API IntTokenExtension(peff::Alloc *allocator, int data);
		SLKC_API virtual ~IntTokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class UIntTokenExtension : public TokenExtension {
	public:
		unsigned int data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API UIntTokenExtension(peff::Alloc *allocator, unsigned int data);
		SLKC_API virtual ~UIntTokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class LongTokenExtension : public TokenExtension {
	public:
		long long data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API LongTokenExtension(peff::Alloc *allocator, long long data);
		SLKC_API virtual ~LongTokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class ULongTokenExtension : public TokenExtension {
	public:
		unsigned long long data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API ULongTokenExtension(peff::Alloc *allocator, unsigned long long data);
		SLKC_API virtual ~ULongTokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class F32TokenExtension : public TokenExtension {
	public:
		float data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API F32TokenExtension(peff::Alloc *allocator, float data);
		SLKC_API virtual ~F32TokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class F64TokenExtension : public TokenExtension {
	public:
		double data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API F64TokenExtension(peff::Alloc *allocator, double data);
		SLKC_API virtual ~F64TokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class StringTokenExtension : public TokenExtension {
	public:
		peff::String data;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API StringTokenExtension(peff::Alloc *allocator, peff::String &&data);
		SLKC_API virtual ~StringTokenExtension();

		SLKC_API virtual void dealloc() override;
	};

	class Document;

	class Token {
	public:
		TokenId tokenId;
		peff::RcObjectPtr<peff::Alloc> allocator;
		std::string_view sourceText;
		peff::WeakPtr<Document> document;
		SourceLocation sourceLocation;
		std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>> exData;
		size_t index = SIZE_MAX;

		SLKC_API Token(peff::Alloc *allocator, const peff::WeakPtr<Document> &document);
		SLKC_API virtual ~Token();

		SLKC_API void dealloc();
	};

	using OwnedTokenPtr = std::unique_ptr<Token, peff::DeallocableDeleter<Token>>;
	using TokenList = peff::DynArray<OwnedTokenPtr>;

	enum class LexicalErrorKind {
		UnrecognizedToken = 0,
		UnexpectedEndOfLine,
		PrematuredEndOfFile,
		OutOfMemory
	};

	struct LexicalError {
		SourceLocation location;
		LexicalErrorKind kind;
	};

	class Lexer {
	public:
		TokenList tokenList;
		peff::Option<LexicalError> lexicalError;

		SLAKE_FORCEINLINE Lexer(peff::Alloc *allocator) : tokenList(allocator) {
		}
		[[nodiscard]] SLKC_API peff::Option<LexicalError> lex(const std::string_view &src, peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
	};

	SLKC_API const char *getTokenName(TokenId tokenId);
}

#endif
