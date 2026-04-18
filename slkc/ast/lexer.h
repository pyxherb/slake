#ifndef _SLKC_AST_LEXER_H_
#define _SLKC_AST_LEXER_H_

#include <slkc/basedefs.h>
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

	class ModuleNode;

	struct SourceLocation {
		ModuleNode *module_node;
		SourcePosition begin_position, end_position;
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
		HashTag,
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
		ShlAssignOp,
		ShrAssignOp,
		StrictEqOp,
		StrictNeqOp,
		EqOp,
		NeqOp,
		ShlOp,
		ShrOp,
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
		DefKeyword,
		DefaultKeyword,
		DoKeyword,
		ElseKeyword,
		EnumKeyword,
		FalseKeyword,
		FnKeyword,
		ForKeyword,
		FinalKeyword,
		FriendKeyword,
		IfKeyword,
		ImportKeyword,
		InKeyword,
		InterfaceKeyword,
		LetKeyword,
		LocalKeyword,
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
		PrivateKeyword,
		ProtectedKeyword,
		ReturnKeyword,
		StaticKeyword,
		StructKeyword,
		SwitchKeyword,
		ThisKeyword,
		ThrowKeyword,
		TypeofKeyword,
		TrueKeyword,
		TryKeyword,
		TypenameKeyword,
		UsingKeyword,
		UnionKeyword,
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
		NeverTypeName,

		I8Literal,
		I16Literal,
		I32Literal,
		I64Literal,
		U8Literal,
		U16Literal,
		U32Literal,
		U64Literal,
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

	SLAKE_FORCEINLINE bool is_valid_token(TokenId token_id) {
		return (((int)token_id) >= 0) && (((int)token_id) < (int)TokenId::MaxToken);
	}

	class TokenExtension {
	public:
		SLKC_API virtual ~TokenExtension();

		virtual void dealloc() = 0;
	};

	enum class IntTokenType {
		Decimal = 0,
		Hexadecimal,
		Octal,
		Binary,
	};

	class IntTokenExtension : public TokenExtension {
	public:
		IntTokenType token_type;
		peff::RcObjectPtr<peff::Alloc> allocator;

		SLKC_API IntTokenExtension(peff::Alloc *allocator, IntTokenType token_type);
		SLKC_API virtual ~IntTokenExtension();

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
		TokenId token_id;
		peff::RcObjectPtr<peff::Alloc> allocator;
		std::string_view source_text;
		peff::WeakPtr<Document> document;
		SourceLocation source_location;
		std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>> ex_data;
		size_t index = SIZE_MAX;

		SLKC_API Token(peff::Alloc *allocator, const peff::WeakPtr<Document> &document);
		SLKC_API ~Token();

		SLKC_API void dealloc();
	};

	using OwnedTokenPtr = peff::UniquePtr<Token, peff::DeallocableDeleter<Token>>;
	using TokenList = peff::DynArray<OwnedTokenPtr>;

	enum class LexicalErrorKind {
		UnrecognizedToken = 0,
		UnexpectedEndOfLine,
		PrematuredEndOfFile,
		InvalidEscape,
		OutOfMemory
	};

	struct LexicalError {
		SourceLocation location;
		LexicalErrorKind kind;
	};

	class Lexer {
	public:
		TokenList token_list;
		peff::Option<LexicalError> lexical_error;

		SLAKE_FORCEINLINE Lexer(peff::Alloc *allocator) : token_list(allocator) {
		}
		[[nodiscard]] SLKC_API peff::Option<LexicalError> lex(ModuleNode *module_node, const std::string_view &src, peff::Alloc *allocator, const peff::SharedPtr<Document> &document);
	};

	SLKC_API const char *get_token_name(TokenId token_id);
}

#endif
