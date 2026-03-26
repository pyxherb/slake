#include <slkc/ast/lexer.h>
#include <algorithm>

using namespace slkc;

enum LexCondition {
	yycInitialCondition = 0,

	yycStringCondition,
	yycEscapeCondition,

	yycCommentCondition,
	yycLineCommentCondition,
};

SLKC_API peff::Option<LexicalError> Lexer::lex(ModuleNode *module_node, const std::string_view &src, peff::Alloc *allocator, const peff::SharedPtr<Document> &document) {
	const char *YYCURSOR = src.data(), *YYMARKER = YYCURSOR, *YYLIMIT = src.data() + src.size();
	const char *prev_YYCURSOR = YYCURSOR;

	LexCondition YYCONDITION = yycInitialCondition;

#define YYSETCONDITION(cond) (YYCONDITION = (yyc##cond))
#define YYGETCONDITION() (YYCONDITION)

	OwnedTokenPtr token;

	while (true) {
		peff::String str_literal(allocator);

		if (!(token = OwnedTokenPtr(peff::alloc_and_construct<Token>(allocator, sizeof(std::max_align_t), allocator, peff::WeakPtr<Document>(document)))))
			goto out_of_memory;

		while (true) {
			/*!re2c
				re2c:yyfill:enable = 0;
				re2c:define:YYCTYPE = char;
				re2c:eof = 1;

				<InitialCondition>"///"		{ YYSETCONDITION(LineCommentCondition); token->token_id = TokenId::DocumentationComment; continue; }
				<InitialCondition>"//"		{ YYSETCONDITION(LineCommentCondition); token->token_id = TokenId::LineComment; continue; }
				<InitialCondition>"/*"		{ YYSETCONDITION(CommentCondition); token->token_id = TokenId::BlockComment; continue; }

				<InitialCondition>"<:"		{ token->token_id = TokenId::SubtypeOp; break; }
				<InitialCondition>"->"		{ token->token_id = TokenId::ReturnTypeOp; break; }
				<InitialCondition>"::"		{ token->token_id = TokenId::ScopeOp; break; }
				<InitialCondition>"=>"		{ token->token_id = TokenId::MatchOp; break; }
				<InitialCondition>"&&"		{ token->token_id = TokenId::LAndOp; break; }
				<InitialCondition>"||"		{ token->token_id = TokenId::LOrOp; break; }
				<InitialCondition>"+"		{ token->token_id = TokenId::AddOp; break; }
				<InitialCondition>"-"		{ token->token_id = TokenId::SubOp; break; }
				<InitialCondition>"*"		{ token->token_id = TokenId::MulOp; break; }
				<InitialCondition>"/"		{ token->token_id = TokenId::DivOp; break; }
				<InitialCondition>"%"		{ token->token_id = TokenId::ModOp; break; }
				<InitialCondition>"&"		{ token->token_id = TokenId::AndOp; break; }
				<InitialCondition>"|"		{ token->token_id = TokenId::OrOp; break; }
				<InitialCondition>"^"		{ token->token_id = TokenId::XorOp; break; }
				<InitialCondition>"!"		{ token->token_id = TokenId::LNotOp; break; }
				<InitialCondition>"~"		{ token->token_id = TokenId::NotOp; break; }
				<InitialCondition>"="		{ token->token_id = TokenId::AssignOp; break; }
				<InitialCondition>"+="		{ token->token_id = TokenId::AddAssignOp; break; }
				<InitialCondition>"-="		{ token->token_id = TokenId::SubAssignOp; break; }
				<InitialCondition>"*="		{ token->token_id = TokenId::MulAssignOp; break; }
				<InitialCondition>"/="		{ token->token_id = TokenId::DivAssignOp; break; }
				<InitialCondition>"%="		{ token->token_id = TokenId::ModAssignOp; break; }
				<InitialCondition>"&="		{ token->token_id = TokenId::AndAssignOp; break; }
				<InitialCondition>"|="		{ token->token_id = TokenId::OrAssignOp; break; }
				<InitialCondition>"^="		{ token->token_id = TokenId::XorAssignOp; break; }
				<InitialCondition>"<<="		{ token->token_id = TokenId::ShlAssignOp; break; }
				<InitialCondition>">>="		{ token->token_id = TokenId::ShrAssignOp; break; }
				<InitialCondition>"==="		{ token->token_id = TokenId::StrictEqOp; break; }
				<InitialCondition>"!=="		{ token->token_id = TokenId::StrictNeqOp; break; }
				<InitialCondition>"=="		{ token->token_id = TokenId::EqOp; break; }
				<InitialCondition>"!="		{ token->token_id = TokenId::NeqOp; break; }
				<InitialCondition>"<<"		{ token->token_id = TokenId::ShlOp; break; }
				<InitialCondition>">>"		{ token->token_id = TokenId::ShrOp; break; }
				<InitialCondition>"<=>"		{ token->token_id = TokenId::CmpOp; break; }
				<InitialCondition>"<="		{ token->token_id = TokenId::LtEqOp; break; }
				<InitialCondition>">="		{ token->token_id = TokenId::GtEqOp; break; }
				<InitialCondition>"<"		{ token->token_id = TokenId::LtOp; break; }
				<InitialCondition>">"		{ token->token_id = TokenId::GtOp; break; }
				<InitialCondition>"$"		{ token->token_id = TokenId::DollarOp; break; }
				<InitialCondition>"@"		{ token->token_id = TokenId::At; break; }

				<InitialCondition>"abstract"	{ token->token_id = TokenId::AbstractKeyword; break; }
				<InitialCondition>"alloca"		{ token->token_id = TokenId::AllocaKeyword; break; }
				<InitialCondition>"as"			{ token->token_id = TokenId::AsKeyword; break; }
				<InitialCondition>"attribute"	{ token->token_id = TokenId::AttributeKeyword; break; }
				<InitialCondition>"async"		{ token->token_id = TokenId::AsyncKeyword; break; }
				<InitialCondition>"await"		{ token->token_id = TokenId::AwaitKeyword; break; }
				<InitialCondition>"base"		{ token->token_id = TokenId::BaseKeyword; break; }
				<InitialCondition>"break"		{ token->token_id = TokenId::BreakKeyword; break; }
				<InitialCondition>"case"		{ token->token_id = TokenId::CaseKeyword; break; }
				<InitialCondition>"catch"		{ token->token_id = TokenId::CatchKeyword; break; }
				<InitialCondition>"class"		{ token->token_id = TokenId::ClassKeyword; break; }
				<InitialCondition>"continue"	{ token->token_id = TokenId::ContinueKeyword; break; }
				<InitialCondition>"const"		{ token->token_id = TokenId::ConstKeyword; break; }
				<InitialCondition>"delete"		{ token->token_id = TokenId::DeleteKeyword; break; }
				<InitialCondition>"default"		{ token->token_id = TokenId::DefaultKeyword; break; }
				<InitialCondition>"def"			{ token->token_id = TokenId::DefKeyword; break; }
				<InitialCondition>"do"			{ token->token_id = TokenId::DoKeyword; break; }
				<InitialCondition>"else"		{ token->token_id = TokenId::ElseKeyword; break; }
				<InitialCondition>"enum"		{ token->token_id = TokenId::EnumKeyword; break; }
				<InitialCondition>"false"		{ token->token_id = TokenId::FalseKeyword; break; }
				<InitialCondition>"fn"			{ token->token_id = TokenId::FnKeyword; break; }
				<InitialCondition>"for"			{ token->token_id = TokenId::ForKeyword; break; }
				<InitialCondition>"final"		{ token->token_id = TokenId::FinalKeyword; break; }
				<InitialCondition>"friend"		{ token->token_id = TokenId::FriendKeyword; break; }
				<InitialCondition>"if"			{ token->token_id = TokenId::IfKeyword; break; }
				<InitialCondition>"import"		{ token->token_id = TokenId::ImportKeyword; break; }
				<InitialCondition>"in"			{ token->token_id = TokenId::InKeyword; break; }
				<InitialCondition>"let"			{ token->token_id = TokenId::LetKeyword; break; }
				<InitialCondition>"local"		{ token->token_id = TokenId::LocalKeyword; break; }
				<InitialCondition>"macro"		{ token->token_id = TokenId::MacroKeyword; break; }
				<InitialCondition>"match"		{ token->token_id = TokenId::MatchKeyword; break; }
				<InitialCondition>"module"		{ token->token_id = TokenId::ModuleKeyword; break; }
				<InitialCondition>"native"		{ token->token_id = TokenId::NativeKeyword; break; }
				<InitialCondition>"new"			{ token->token_id = TokenId::NewKeyword; break; }
				<InitialCondition>"null"		{ token->token_id = TokenId::NullKeyword; break; }
				<InitialCondition>"out"			{ token->token_id = TokenId::OutKeyword; break; }
				<InitialCondition>"operator"	{ token->token_id = TokenId::OperatorKeyword; break; }
				<InitialCondition>"override"	{ token->token_id = TokenId::OverrideKeyword; break; }
				<InitialCondition>"public"		{ token->token_id = TokenId::PublicKeyword; break; }
				<InitialCondition>"return"		{ token->token_id = TokenId::ReturnKeyword; break; }
				<InitialCondition>"static"		{ token->token_id = TokenId::StaticKeyword; break; }
				<InitialCondition>"struct"		{ token->token_id = TokenId::StructKeyword; break; }
				<InitialCondition>"switch"		{ token->token_id = TokenId::SwitchKeyword; break; }
				<InitialCondition>"this"		{ token->token_id = TokenId::ThisKeyword; break; }
				<InitialCondition>"throw"		{ token->token_id = TokenId::ThrowKeyword; break; }
				<InitialCondition>"typeof"		{ token->token_id = TokenId::TypeofKeyword; break; }
				<InitialCondition>"interface"	{ token->token_id = TokenId::InterfaceKeyword; break; }
				<InitialCondition>"true"		{ token->token_id = TokenId::TrueKeyword; break; }
				<InitialCondition>"try"			{ token->token_id = TokenId::TryKeyword; break; }
				<InitialCondition>"typename"	{ token->token_id = TokenId::TypenameKeyword; break; }
				<InitialCondition>"using"		{ token->token_id = TokenId::UsingKeyword; break; }
				<InitialCondition>"union"		{ token->token_id = TokenId::UnionKeyword; break; }
				<InitialCondition>"var"			{ token->token_id = TokenId::VarKeyword; break; }
				<InitialCondition>"virtual"		{ token->token_id = TokenId::VirtualKeyword; break; }
				<InitialCondition>"with"		{ token->token_id = TokenId::WithKeyword; break; }
				<InitialCondition>"while"		{ token->token_id = TokenId::WhileKeyword; break; }
				<InitialCondition>"yield"		{ token->token_id = TokenId::YieldKeyword; break; }

				<InitialCondition>"i8"			{ token->token_id = TokenId::I8TypeName; break; }
				<InitialCondition>"i16"			{ token->token_id = TokenId::I16TypeName; break; }
				<InitialCondition>"i32"			{ token->token_id = TokenId::I32TypeName; break; }
				<InitialCondition>"i64"			{ token->token_id = TokenId::I64TypeName; break; }
				<InitialCondition>"isize"		{ token->token_id = TokenId::ISizeTypeName; break; }
				<InitialCondition>"u8"			{ token->token_id = TokenId::U8TypeName; break; }
				<InitialCondition>"u16"			{ token->token_id = TokenId::U16TypeName; break; }
				<InitialCondition>"u32"			{ token->token_id = TokenId::U32TypeName; break; }
				<InitialCondition>"u64"			{ token->token_id = TokenId::U64TypeName; break; }
				<InitialCondition>"usize"		{ token->token_id = TokenId::USizeTypeName; break; }
				<InitialCondition>"f32"			{ token->token_id = TokenId::F32TypeName; break; }
				<InitialCondition>"f64"			{ token->token_id = TokenId::F64TypeName; break; }
				<InitialCondition>"string"		{ token->token_id = TokenId::StringTypeName; break; }
				<InitialCondition>"bool"		{ token->token_id = TokenId::BoolTypeName; break; }
				<InitialCondition>"auto"		{ token->token_id = TokenId::AutoTypeName; break; }
				<InitialCondition>"void"		{ token->token_id = TokenId::VoidTypeName; break; }
				<InitialCondition>"object"		{ token->token_id = TokenId::ObjectTypeName; break; }
				<InitialCondition>"any"			{ token->token_id = TokenId::AnyTypeName; break; }
				<InitialCondition>"simd_t"		{ token->token_id = TokenId::SIMDTypeName; break; }
				<InitialCondition>"no_return"	{ token->token_id = TokenId::NoReturnTypeName; break; }

				<InitialCondition>","		{ token->token_id = TokenId::Comma; break; }
				<InitialCondition>"?"		{ token->token_id = TokenId::Question; break; }
				<InitialCondition>":"		{ token->token_id = TokenId::Colon; break; }
				<InitialCondition>";"     	{ token->token_id = TokenId::Semicolon; break; }
				<InitialCondition>"[["		{ token->token_id = TokenId::LDBracket; break; }
				<InitialCondition>"]]"		{ token->token_id = TokenId::RDBracket; break; }
				<InitialCondition>"["		{ token->token_id = TokenId::LBracket; break; }
				<InitialCondition>"]"		{ token->token_id = TokenId::RBracket; break; }
				<InitialCondition>"{"		{ token->token_id = TokenId::LBrace; break; }
				<InitialCondition>"}"		{ token->token_id = TokenId::RBrace; break; }
				<InitialCondition>"("		{ token->token_id = TokenId::LParenthese; break; }
				<InitialCondition>")"		{ token->token_id = TokenId::RParenthese; break; }
				<InitialCondition>"#"		{ token->token_id = TokenId::HashTag; break; }
				<InitialCondition>"..."		{ token->token_id = TokenId::VarArg; break; }
				<InitialCondition>"."		{ token->token_id = TokenId::Dot; break; }

				<InitialCondition>[a-zA-Z_\x80-\xff][a-zA-Z0-9_\x80-\xff]* {
					token->token_id = TokenId::Id;
					break;
				}

				<InitialCondition>"0"[0-7]+[uU][lL] {
					token->token_id = TokenId::U64Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Octal));
					break;
				}

				<InitialCondition>[0-9]+[uU][lL] {
					token->token_id = TokenId::U64Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Decimal));
					break;
				}

				<InitialCondition>"0"[xX][0-9a-fA-F]+[uU][lL] {
					token->token_id = TokenId::U64Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Hexadecimal));
					break;
				}

				<InitialCondition>"0"[bB][01]+[uU][lL] {
					token->token_id = TokenId::U64Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Binary));
					break;
				}

				<InitialCondition>"0"[0-7]+[uU] {
					token->token_id = TokenId::U32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Octal));
					break;
				}

				<InitialCondition>[0-9]+[uU] {
					token->token_id = TokenId::U32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Decimal));
					break;
				}

				<InitialCondition>"0"[xX][0-9a-fA-F]+[uU] {
					token->token_id = TokenId::U32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Hexadecimal));
					break;
				}

				<InitialCondition>"0"[bB][01]+[uU] {
					token->token_id = TokenId::U32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Binary));
					break;
				}

				<InitialCondition>"0"[0-7]+ {
					token->token_id = TokenId::I32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Octal));
					break;
				}

				<InitialCondition>[0-9]+ {
					token->token_id = TokenId::I32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Decimal));
					break;
				}

				<InitialCondition>"0"[xX][0-9a-fA-F]+ {
					token->token_id = TokenId::I32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Hexadecimal));
					break;
				}

				<InitialCondition>"0"[bB][01]+ {
					token->token_id = TokenId::I32Literal;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<IntTokenExtension>(allocator, alignof(IntTokenExtension), allocator, IntTokenType::Binary));
					break;
				}

				<InitialCondition>[0-9]+"."[0-9]+[fF] {
					token->token_id = TokenId::F32Literal;
					break;
				}

				<InitialCondition>[0-9]+"."[0-9]+ {
					token->token_id = TokenId::F64Literal;
					break;
				}

				<InitialCondition>"\""		{ YYSETCONDITION(StringCondition); continue; }

				<InitialCondition>"\n"		{ token->token_id = TokenId::NewLine; break; }
				<InitialCondition>$			{ goto end; }

				<InitialCondition>[ \r\t]+	{ token->token_id = TokenId::Whitespace; break; }

				<InitialCondition>[^]		{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::UnrecognizedToken};
				}

				<StringCondition>"\""		{
					YYSETCONDITION(InitialCondition);
					token->token_id = TokenId::StringLiteral;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<StringTokenExtension>(allocator, sizeof(std::max_align_t), allocator, std::move(str_literal)));
					break;
				}
				<StringCondition>"\\"		{ YYSETCONDITION(EscapeCondition); continue; }
				<StringCondition>"\n"		{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::UnexpectedEndOfLine};
				}
				<StringCondition>$	{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::PrematuredEndOfFile};
				}
				<StringCondition>[^]		{ if(!str_literal.push_back(+YYCURSOR[-1])) goto out_of_memory; continue; }

				<EscapeCondition>"'"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\'')) goto out_of_memory; continue; }
				<EscapeCondition>"\""	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('"')) goto out_of_memory; continue; }
				<EscapeCondition>"?"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('?')) goto out_of_memory; continue; }
				<EscapeCondition>"\\"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\\')) goto out_of_memory; continue; }
				<EscapeCondition>"a"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\a')) goto out_of_memory; continue; }
				<EscapeCondition>"b"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\b')) goto out_of_memory; continue; }
				<EscapeCondition>"f"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\f')) goto out_of_memory; continue; }
				<EscapeCondition>"n"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\n')) goto out_of_memory; continue; }
				<EscapeCondition>"r"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\r')) goto out_of_memory; continue; }
				<EscapeCondition>"t"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\t')) goto out_of_memory; continue; }
				<EscapeCondition>"v"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\v')) goto out_of_memory; continue; }
				<EscapeCondition>[0-7]{1,3}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prev_YYCURSOR;

					char c = 0;
					for(uint_fast8_t i = 0; i < size; ++i) {
						c *= 8;
						c += prev_YYCURSOR[i] - '0';
					}

					if(!str_literal.push_back(+c))
						goto out_of_memory;
				}
				<EscapeCondition>[xX][0-9a-fA-F]{1,2}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prev_YYCURSOR;

					char c = 0, j;

					for(uint_fast8_t i = 1; i < size; ++i) {
						c *= 16;

						j = prev_YYCURSOR[i];
						if((j >= '0') && (j <= '9'))
							c += prev_YYCURSOR[i] - '0';
						else if((j >= 'a') && (j <= 'f'))
							c += prev_YYCURSOR[i] - 'a';
						else if((j >= 'A') && (j <= 'F'))
							c += prev_YYCURSOR[i] - 'A';
					}

					if(!str_literal.push_back(+c))
						goto out_of_memory;
				}
				<EscapeCondition>$	{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::PrematuredEndOfFile};
				}
				<EscapeCondition>[^]{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::PrematuredEndOfFile};
				}

				<CommentCondition>"*"[/]	{ YYSETCONDITION(InitialCondition); break; }
				<CommentCondition>[^]		{ continue; }
				<CommentCondition>$	{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::InvalidEscape};
				}

				<LineCommentCondition>"\n"	{ YYSETCONDITION(InitialCondition); break; }
				<LineCommentCondition>$		{ YYSETCONDITION(InitialCondition); break; }
				<LineCommentCondition>[^]	{ continue; }
			*/
		}

		size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();

		std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

		token->source_text = std::string_view(prev_YYCURSOR, YYCURSOR - prev_YYCURSOR);

		size_t idxLastBeginNewline = src.find_last_of('\n', begin_index),
			   idxLastEndNewline = src.find_last_of('\n', endIndex);

		token->source_location.module_node = module_node;
		token->source_location.begin_position = {
			(size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'),
			(idxLastBeginNewline == std::string::npos
					? begin_index
					: begin_index - idxLastBeginNewline - 1)
		};
		token->source_location.end_position = {
			(size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'),
			(idxLastEndNewline == std::string::npos
					? endIndex
					: endIndex - idxLastEndNewline)
		};
		if (!token_list.push_back(std::move(token)))
			goto out_of_memory;

		prev_YYCURSOR = YYCURSOR;
	}

end: {
	SourceLocation endLocation = token->source_location;

	token = OwnedTokenPtr(peff::alloc_and_construct<Token>(allocator, sizeof(std::max_align_t), allocator, document));
	token->token_id = TokenId::End;
	token->source_location = endLocation;

	if (!token_list.push_back(std::move(token)))
		goto out_of_memory;
}

	return {};

out_of_memory:
	return LexicalError{ SourceLocation{ module_node, { 0, 0 }, { 0, 0 } }, LexicalErrorKind::OutOfMemory };
}
