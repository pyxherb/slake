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

SLKC_API peff::Option<LexicalError> Lexer::lex(ModuleNode *moduleNode, const std::string_view &src, peff::Alloc *allocator, const peff::SharedPtr<Document> &document) {
	const char *YYCURSOR = src.data(), *YYMARKER = YYCURSOR, *YYLIMIT = src.data() + src.size();
	const char *prevYYCURSOR = YYCURSOR;

	LexCondition YYCONDITION = yycInitialCondition;

#define YYSETCONDITION(cond) (YYCONDITION = (yyc##cond))
#define YYGETCONDITION() (YYCONDITION)

	OwnedTokenPtr token;

	while (true) {
		peff::String strLiteral(allocator);

		if (!(token = OwnedTokenPtr(peff::allocAndConstruct<Token>(allocator, sizeof(std::max_align_t), allocator, peff::WeakPtr<Document>(document)))))
			goto outOfMemory;

		while (true) {
			/*!re2c
				re2c:yyfill:enable = 0;
				re2c:define:YYCTYPE = char;
				re2c:eof = 1;

				<InitialCondition>"///"		{ YYSETCONDITION(LineCommentCondition); token->tokenId = TokenId::DocumentationComment; continue; }
				<InitialCondition>"//"		{ YYSETCONDITION(LineCommentCondition); token->tokenId = TokenId::LineComment; continue; }
				<InitialCondition>"/*"		{ YYSETCONDITION(CommentCondition); token->tokenId = TokenId::BlockComment; continue; }

				<InitialCondition>"<:"		{ token->tokenId = TokenId::SubtypeOp; break; }
				<InitialCondition>"->"		{ token->tokenId = TokenId::ReturnTypeOp; break; }
				<InitialCondition>"::"		{ token->tokenId = TokenId::ScopeOp; break; }
				<InitialCondition>"=>"		{ token->tokenId = TokenId::MatchOp; break; }
				<InitialCondition>"&&"		{ token->tokenId = TokenId::LAndOp; break; }
				<InitialCondition>"||"		{ token->tokenId = TokenId::LOrOp; break; }
				<InitialCondition>"+"		{ token->tokenId = TokenId::AddOp; break; }
				<InitialCondition>"-"		{ token->tokenId = TokenId::SubOp; break; }
				<InitialCondition>"*"		{ token->tokenId = TokenId::MulOp; break; }
				<InitialCondition>"/"		{ token->tokenId = TokenId::DivOp; break; }
				<InitialCondition>"%"		{ token->tokenId = TokenId::ModOp; break; }
				<InitialCondition>"&"		{ token->tokenId = TokenId::AndOp; break; }
				<InitialCondition>"|"		{ token->tokenId = TokenId::OrOp; break; }
				<InitialCondition>"^"		{ token->tokenId = TokenId::XorOp; break; }
				<InitialCondition>"!"		{ token->tokenId = TokenId::LNotOp; break; }
				<InitialCondition>"~"		{ token->tokenId = TokenId::NotOp; break; }
				<InitialCondition>"="		{ token->tokenId = TokenId::AssignOp; break; }
				<InitialCondition>"+="		{ token->tokenId = TokenId::AddAssignOp; break; }
				<InitialCondition>"-="		{ token->tokenId = TokenId::SubAssignOp; break; }
				<InitialCondition>"*="		{ token->tokenId = TokenId::MulAssignOp; break; }
				<InitialCondition>"/="		{ token->tokenId = TokenId::DivAssignOp; break; }
				<InitialCondition>"%="		{ token->tokenId = TokenId::ModAssignOp; break; }
				<InitialCondition>"&="		{ token->tokenId = TokenId::AndAssignOp; break; }
				<InitialCondition>"|="		{ token->tokenId = TokenId::OrAssignOp; break; }
				<InitialCondition>"^="		{ token->tokenId = TokenId::XorAssignOp; break; }
				<InitialCondition>"<<="		{ token->tokenId = TokenId::LshAssignOp; break; }
				<InitialCondition>">>="		{ token->tokenId = TokenId::RshAssignOp; break; }
				<InitialCondition>"==="		{ token->tokenId = TokenId::StrictEqOp; break; }
				<InitialCondition>"!=="		{ token->tokenId = TokenId::StrictNeqOp; break; }
				<InitialCondition>"=="		{ token->tokenId = TokenId::EqOp; break; }
				<InitialCondition>"!="		{ token->tokenId = TokenId::NeqOp; break; }
				<InitialCondition>"<<"		{ token->tokenId = TokenId::LshOp; break; }
				<InitialCondition>">>"		{ token->tokenId = TokenId::RshOp; break; }
				<InitialCondition>"<=>"		{ token->tokenId = TokenId::CmpOp; break; }
				<InitialCondition>"<="		{ token->tokenId = TokenId::LtEqOp; break; }
				<InitialCondition>">="		{ token->tokenId = TokenId::GtEqOp; break; }
				<InitialCondition>"<"		{ token->tokenId = TokenId::LtOp; break; }
				<InitialCondition>">"		{ token->tokenId = TokenId::GtOp; break; }
				<InitialCondition>"$"		{ token->tokenId = TokenId::DollarOp; break; }
				<InitialCondition>"@"		{ token->tokenId = TokenId::At; break; }

				<InitialCondition>"abstract"	{ token->tokenId = TokenId::AbstractKeyword; break; }
				<InitialCondition>"alloca"		{ token->tokenId = TokenId::AllocaKeyword; break; }
				<InitialCondition>"as"			{ token->tokenId = TokenId::AsKeyword; break; }
				<InitialCondition>"attribute"	{ token->tokenId = TokenId::AttributeKeyword; break; }
				<InitialCondition>"async"		{ token->tokenId = TokenId::AsyncKeyword; break; }
				<InitialCondition>"await"		{ token->tokenId = TokenId::AwaitKeyword; break; }
				<InitialCondition>"base"		{ token->tokenId = TokenId::BaseKeyword; break; }
				<InitialCondition>"break"		{ token->tokenId = TokenId::BreakKeyword; break; }
				<InitialCondition>"case"		{ token->tokenId = TokenId::CaseKeyword; break; }
				<InitialCondition>"catch"		{ token->tokenId = TokenId::CatchKeyword; break; }
				<InitialCondition>"class"		{ token->tokenId = TokenId::ClassKeyword; break; }
				<InitialCondition>"continue"	{ token->tokenId = TokenId::ContinueKeyword; break; }
				<InitialCondition>"const"		{ token->tokenId = TokenId::ConstKeyword; break; }
				<InitialCondition>"delete"		{ token->tokenId = TokenId::DeleteKeyword; break; }
				<InitialCondition>"default"		{ token->tokenId = TokenId::DefaultKeyword; break; }
				<InitialCondition>"def"			{ token->tokenId = TokenId::DefKeyword; break; }
				<InitialCondition>"do"			{ token->tokenId = TokenId::DoKeyword; break; }
				<InitialCondition>"else"		{ token->tokenId = TokenId::ElseKeyword; break; }
				<InitialCondition>"enum"		{ token->tokenId = TokenId::EnumKeyword; break; }
				<InitialCondition>"false"		{ token->tokenId = TokenId::FalseKeyword; break; }
				<InitialCondition>"fn"			{ token->tokenId = TokenId::FnKeyword; break; }
				<InitialCondition>"for"			{ token->tokenId = TokenId::ForKeyword; break; }
				<InitialCondition>"final"		{ token->tokenId = TokenId::FinalKeyword; break; }
				<InitialCondition>"friend"		{ token->tokenId = TokenId::FriendKeyword; break; }
				<InitialCondition>"if"			{ token->tokenId = TokenId::IfKeyword; break; }
				<InitialCondition>"import"		{ token->tokenId = TokenId::ImportKeyword; break; }
				<InitialCondition>"in"			{ token->tokenId = TokenId::InKeyword; break; }
				<InitialCondition>"let"			{ token->tokenId = TokenId::LetKeyword; break; }
				<InitialCondition>"local"		{ token->tokenId = TokenId::LocalKeyword; break; }
				<InitialCondition>"macro"		{ token->tokenId = TokenId::MacroKeyword; break; }
				<InitialCondition>"match"		{ token->tokenId = TokenId::MatchKeyword; break; }
				<InitialCondition>"module"		{ token->tokenId = TokenId::ModuleKeyword; break; }
				<InitialCondition>"native"		{ token->tokenId = TokenId::NativeKeyword; break; }
				<InitialCondition>"new"			{ token->tokenId = TokenId::NewKeyword; break; }
				<InitialCondition>"null"		{ token->tokenId = TokenId::NullKeyword; break; }
				<InitialCondition>"out"			{ token->tokenId = TokenId::OutKeyword; break; }
				<InitialCondition>"operator"	{ token->tokenId = TokenId::OperatorKeyword; break; }
				<InitialCondition>"override"	{ token->tokenId = TokenId::OverrideKeyword; break; }
				<InitialCondition>"public"		{ token->tokenId = TokenId::PublicKeyword; break; }
				<InitialCondition>"return"		{ token->tokenId = TokenId::ReturnKeyword; break; }
				<InitialCondition>"static"		{ token->tokenId = TokenId::StaticKeyword; break; }
				<InitialCondition>"struct"		{ token->tokenId = TokenId::StructKeyword; break; }
				<InitialCondition>"switch"		{ token->tokenId = TokenId::SwitchKeyword; break; }
				<InitialCondition>"this"		{ token->tokenId = TokenId::ThisKeyword; break; }
				<InitialCondition>"throw"		{ token->tokenId = TokenId::ThrowKeyword; break; }
				<InitialCondition>"typeof"		{ token->tokenId = TokenId::TypeofKeyword; break; }
				<InitialCondition>"interface"	{ token->tokenId = TokenId::InterfaceKeyword; break; }
				<InitialCondition>"true"		{ token->tokenId = TokenId::TrueKeyword; break; }
				<InitialCondition>"try"			{ token->tokenId = TokenId::TryKeyword; break; }
				<InitialCondition>"typename"	{ token->tokenId = TokenId::TypenameKeyword; break; }
				<InitialCondition>"using"		{ token->tokenId = TokenId::UsingKeyword; break; }
				<InitialCondition>"union"		{ token->tokenId = TokenId::UnionKeyword; break; }
				<InitialCondition>"var"			{ token->tokenId = TokenId::VarKeyword; break; }
				<InitialCondition>"virtual"		{ token->tokenId = TokenId::VirtualKeyword; break; }
				<InitialCondition>"with"		{ token->tokenId = TokenId::WithKeyword; break; }
				<InitialCondition>"while"		{ token->tokenId = TokenId::WhileKeyword; break; }
				<InitialCondition>"yield"		{ token->tokenId = TokenId::YieldKeyword; break; }

				<InitialCondition>"i8"			{ token->tokenId = TokenId::I8TypeName; break; }
				<InitialCondition>"i16"			{ token->tokenId = TokenId::I16TypeName; break; }
				<InitialCondition>"i32"			{ token->tokenId = TokenId::I32TypeName; break; }
				<InitialCondition>"i64"			{ token->tokenId = TokenId::I64TypeName; break; }
				<InitialCondition>"isize"		{ token->tokenId = TokenId::ISizeTypeName; break; }
				<InitialCondition>"u8"			{ token->tokenId = TokenId::U8TypeName; break; }
				<InitialCondition>"u16"			{ token->tokenId = TokenId::U16TypeName; break; }
				<InitialCondition>"u32"			{ token->tokenId = TokenId::U32TypeName; break; }
				<InitialCondition>"u64"			{ token->tokenId = TokenId::U64TypeName; break; }
				<InitialCondition>"usize"		{ token->tokenId = TokenId::USizeTypeName; break; }
				<InitialCondition>"f32"			{ token->tokenId = TokenId::F32TypeName; break; }
				<InitialCondition>"f64"			{ token->tokenId = TokenId::F64TypeName; break; }
				<InitialCondition>"string"		{ token->tokenId = TokenId::StringTypeName; break; }
				<InitialCondition>"bool"		{ token->tokenId = TokenId::BoolTypeName; break; }
				<InitialCondition>"auto"		{ token->tokenId = TokenId::AutoTypeName; break; }
				<InitialCondition>"void"		{ token->tokenId = TokenId::VoidTypeName; break; }
				<InitialCondition>"object"		{ token->tokenId = TokenId::ObjectTypeName; break; }
				<InitialCondition>"any"			{ token->tokenId = TokenId::AnyTypeName; break; }
				<InitialCondition>"simd_t"		{ token->tokenId = TokenId::SIMDTypeName; break; }
				<InitialCondition>"no_return"	{ token->tokenId = TokenId::NoReturnTypeName; break; }

				<InitialCondition>","		{ token->tokenId = TokenId::Comma; break; }
				<InitialCondition>"?"		{ token->tokenId = TokenId::Question; break; }
				<InitialCondition>":"		{ token->tokenId = TokenId::Colon; break; }
				<InitialCondition>";"     	{ token->tokenId = TokenId::Semicolon; break; }
				<InitialCondition>"[["		{ token->tokenId = TokenId::LDBracket; break; }
				<InitialCondition>"]]"		{ token->tokenId = TokenId::RDBracket; break; }
				<InitialCondition>"["		{ token->tokenId = TokenId::LBracket; break; }
				<InitialCondition>"]"		{ token->tokenId = TokenId::RBracket; break; }
				<InitialCondition>"{"		{ token->tokenId = TokenId::LBrace; break; }
				<InitialCondition>"}"		{ token->tokenId = TokenId::RBrace; break; }
				<InitialCondition>"("		{ token->tokenId = TokenId::LParenthese; break; }
				<InitialCondition>")"		{ token->tokenId = TokenId::RParenthese; break; }
				<InitialCondition>"#"		{ token->tokenId = TokenId::HashTag; break; }
				<InitialCondition>"..."		{ token->tokenId = TokenId::VarArg; break; }
				<InitialCondition>"."		{ token->tokenId = TokenId::Dot; break; }

				<InitialCondition>[a-zA-Z_][a-zA-Z0-9_]* {
					token->tokenId = TokenId::Id;
					break;
				}

				<InitialCondition>"0"[0-7]+ {
					token->tokenId = TokenId::UIntLiteral;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<UIntTokenExtension>(allocator, sizeof(std::max_align_t), allocator, strtoul(prevYYCURSOR, nullptr, 8)));
					break;
				}

				<InitialCondition>[0-9]+ {
					token->tokenId = TokenId::IntLiteral;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<IntTokenExtension>(allocator, sizeof(std::max_align_t), allocator, strtol(prevYYCURSOR, nullptr, 10)));
					break;
				}

				<InitialCondition>"0"[xX][0-9a-fA-F]+ {
					token->tokenId = TokenId::UIntLiteral;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<UIntTokenExtension>(allocator, sizeof(std::max_align_t), allocator, strtoul(prevYYCURSOR, nullptr, 16)));
					break;
				}

				<InitialCondition>"0"[bB][01]+ {
					token->tokenId = TokenId::UIntLiteral;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<UIntTokenExtension>(allocator, sizeof(std::max_align_t), allocator, strtoul(prevYYCURSOR, nullptr, 2)));
					break;
				}

				<InitialCondition>[0-9]+"."[0-9]+[fF] {
					token->tokenId = TokenId::F32Literal;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<F32TokenExtension>(allocator, sizeof(std::max_align_t), allocator, strtof(prevYYCURSOR, nullptr)));
					break;
				}

				<InitialCondition>[0-9]+"."[0-9]+ {
					token->tokenId = TokenId::F64Literal;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<F64TokenExtension>(allocator, sizeof(std::max_align_t), allocator, strtod(prevYYCURSOR, nullptr)));
					break;
				}

				<InitialCondition>"\""		{ YYSETCONDITION(StringCondition); continue; }

				<InitialCondition>"\n"		{ token->tokenId = TokenId::NewLine; break; }
				<InitialCondition>$			{ goto end; }

				<InitialCondition>[ \r\t]+	{ token->tokenId = TokenId::Whitespace; break; }

				<InitialCondition>[^]		{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t prevYYCURSORIndex = prevYYCURSOR - src.data();
					auto prevYYCURSORPos = src.find_last_of('\n', prevYYCURSORIndex);
					if(prevYYCURSORPos == std::string::npos)
						prevYYCURSORPos = 0;
					prevYYCURSORPos = prevYYCURSORIndex - prevYYCURSORPos;

					size_t YYCURSORIndex = YYCURSOR - src.data();
					auto YYCURSORPos = src.find_last_of('\n', YYCURSORIndex);
					if(YYCURSORPos == std::string::npos)
						YYCURSORPos = 0;
					YYCURSORPos = YYCURSORIndex - YYCURSORPos;

					return LexicalError {
						SourceLocation {
						moduleNode,
						{ (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), prevYYCURSORPos },
						{ (size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'), YYCURSORPos }
					}, LexicalErrorKind::UnrecognizedToken};
				}

				<StringCondition>"\""		{
					YYSETCONDITION(InitialCondition);
					token->tokenId = TokenId::StringLiteral;
					token->exData = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::allocAndConstruct<StringTokenExtension>(allocator, sizeof(std::max_align_t), allocator, std::move(strLiteral)));
					break;
				}
				<StringCondition>"\\"		{ YYSETCONDITION(EscapeCondition); continue; }
				<StringCondition>"\n"		{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t prevYYCURSORIndex = prevYYCURSOR - src.data();
					auto prevYYCURSORPos = src.find_last_of('\n', prevYYCURSORIndex);
					if(prevYYCURSORPos == std::string::npos)
						prevYYCURSORPos = 0;
					prevYYCURSORPos = prevYYCURSORIndex - prevYYCURSORPos;

					size_t YYCURSORIndex = YYCURSOR - src.data();
					auto YYCURSORPos = src.find_last_of('\n', YYCURSORIndex);
					if(YYCURSORPos == std::string::npos)
						YYCURSORPos = 0;
					YYCURSORPos = YYCURSORIndex - YYCURSORPos;

					return LexicalError {
						SourceLocation {
						moduleNode,
						{ (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), prevYYCURSORPos },
						{ (size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'), YYCURSORPos }
					}, LexicalErrorKind::UnexpectedEndOfLine};
				}
				<StringCondition>$	{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t prevYYCURSORIndex = prevYYCURSOR - src.data();
					auto prevYYCURSORPos = src.find_last_of('\n', prevYYCURSORIndex);
					if(prevYYCURSORPos == std::string::npos)
						prevYYCURSORPos = 0;
					prevYYCURSORPos = prevYYCURSORIndex - prevYYCURSORPos;

					size_t YYCURSORIndex = YYCURSOR - src.data();
					auto YYCURSORPos = src.find_last_of('\n', YYCURSORIndex);
					if(YYCURSORPos == std::string::npos)
						YYCURSORPos = 0;
					YYCURSORPos = YYCURSORIndex - YYCURSORPos;

					return LexicalError {
						SourceLocation {
						moduleNode,
						{ (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), prevYYCURSORPos },
						{ (size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'), YYCURSORPos }
					}, LexicalErrorKind::PrematuredEndOfFile};
				}
				<StringCondition>[^]		{ if(!strLiteral.pushBack(+YYCURSOR[-1])) goto outOfMemory; continue; }

				<EscapeCondition>"'"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\'')) goto outOfMemory; continue; }
				<EscapeCondition>"\""	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('"')) goto outOfMemory; continue; }
				<EscapeCondition>"?"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('?')) goto outOfMemory; continue; }
				<EscapeCondition>"\\"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\\')) goto outOfMemory; continue; }
				<EscapeCondition>"a"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\a')) goto outOfMemory; continue; }
				<EscapeCondition>"b"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\b')) goto outOfMemory; continue; }
				<EscapeCondition>"f"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\f')) goto outOfMemory; continue; }
				<EscapeCondition>"n"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\n')) goto outOfMemory; continue; }
				<EscapeCondition>"r"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\r')) goto outOfMemory; continue; }
				<EscapeCondition>"t"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\t')) goto outOfMemory; continue; }
				<EscapeCondition>"v"	{ YYSETCONDITION(StringCondition); if(!strLiteral.pushBack('\v')) goto outOfMemory; continue; }
				<EscapeCondition>[0-7]{1,3}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prevYYCURSOR;

					char c = 0;
					for(uint_fast8_t i = 0; i < size; ++i) {
						c *= 8;
						c += prevYYCURSOR[i] - '0';
					}

					if(!strLiteral.pushBack(+c))
						goto outOfMemory;
				}
				<EscapeCondition>[xX][0-9a-fA-F]{1,2}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prevYYCURSOR;

					char c = 0, j;

					for(uint_fast8_t i = 1; i < size; ++i) {
						c *= 16;

						j = prevYYCURSOR[i];
						if((j >= '0') && (j <= '9'))
							c += prevYYCURSOR[i] - '0';
						else if((j >= 'a') && (j <= 'f'))
							c += prevYYCURSOR[i] - 'a';
						else if((j >= 'A') && (j <= 'F'))
							c += prevYYCURSOR[i] - 'A';
					}

					if(!strLiteral.pushBack(+c))
						goto outOfMemory;
				}
				<EscapeCondition>$	{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t prevYYCURSORIndex = prevYYCURSOR - src.data();
					auto prevYYCURSORPos = src.find_last_of('\n', prevYYCURSORIndex);
					if(prevYYCURSORPos == std::string::npos)
						prevYYCURSORPos = 0;
					prevYYCURSORPos = prevYYCURSORIndex - prevYYCURSORPos;

					size_t YYCURSORIndex = YYCURSOR - src.data();
					auto YYCURSORPos = src.find_last_of('\n', YYCURSORIndex);
					if(YYCURSORPos == std::string::npos)
						YYCURSORPos = 0;
					YYCURSORPos = YYCURSORIndex - YYCURSORPos;

					return LexicalError {
						SourceLocation {
						moduleNode,
						{ (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), prevYYCURSORPos },
						{ (size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'), YYCURSORPos }
					}, LexicalErrorKind::PrematuredEndOfFile};
				}
				<EscapeCondition>[^]{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t prevYYCURSORIndex = prevYYCURSOR - src.data();
					auto prevYYCURSORPos = src.find_last_of('\n', prevYYCURSORIndex);
					if(prevYYCURSORPos == std::string::npos)
						prevYYCURSORPos = 0;
					prevYYCURSORPos = prevYYCURSORIndex - prevYYCURSORPos;

					size_t YYCURSORIndex = YYCURSOR - src.data();
					auto YYCURSORPos = src.find_last_of('\n', YYCURSORIndex);
					if(YYCURSORPos == std::string::npos)
						YYCURSORPos = 0;
					YYCURSORPos = YYCURSORIndex - YYCURSORPos;

					return LexicalError {
						SourceLocation {
						moduleNode,
						{ (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), prevYYCURSORPos },
						{ (size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'), YYCURSORPos }
					}, LexicalErrorKind::PrematuredEndOfFile};
				}

				<CommentCondition>"*"[/]	{ YYSETCONDITION(InitialCondition); break; }
				<CommentCondition>[^]		{ continue; }
				<CommentCondition>$	{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t prevYYCURSORIndex = prevYYCURSOR - src.data();
					auto prevYYCURSORPos = src.find_last_of('\n', prevYYCURSORIndex);
					if(prevYYCURSORPos == std::string::npos)
						prevYYCURSORPos = 0;
					prevYYCURSORPos = prevYYCURSORIndex - prevYYCURSORPos;

					size_t YYCURSORIndex = YYCURSOR - src.data();
					auto YYCURSORPos = src.find_last_of('\n', YYCURSORIndex);
					if(YYCURSORPos == std::string::npos)
						YYCURSORPos = 0;
					YYCURSORPos = YYCURSORIndex - YYCURSORPos;

					return LexicalError {
						SourceLocation {
						moduleNode,
						{ (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), prevYYCURSORPos },
						{ (size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'), YYCURSORPos }
					}, LexicalErrorKind::InvalidEscape};
				}

				<LineCommentCondition>"\n"	{ YYSETCONDITION(InitialCondition); break; }
				<LineCommentCondition>$		{ YYSETCONDITION(InitialCondition); break; }
				<LineCommentCondition>[^]	{ continue; }
			*/
		}

		size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();

		std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

		token->sourceText = std::string_view(prevYYCURSOR, YYCURSOR - prevYYCURSOR);

		size_t idxLastBeginNewline = src.find_last_of('\n', beginIndex),
			   idxLastEndNewline = src.find_last_of('\n', endIndex);

		token->sourceLocation.moduleNode = moduleNode;
		token->sourceLocation.beginPosition = {
			(size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'),
			(idxLastBeginNewline == std::string::npos
					? beginIndex
					: beginIndex - idxLastBeginNewline - 1)
		};
		token->sourceLocation.endPosition = {
			(size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'),
			(idxLastEndNewline == std::string::npos
					? endIndex
					: endIndex - idxLastEndNewline)
		};
		if (!tokenList.pushBack(std::move(token)))
			goto outOfMemory;

		prevYYCURSOR = YYCURSOR;
	}

end: {
	SourceLocation endLocation = token->sourceLocation;

	token = OwnedTokenPtr(peff::allocAndConstruct<Token>(allocator, sizeof(std::max_align_t), allocator, document));
	token->tokenId = TokenId::End;
	token->sourceLocation = endLocation;

	if (!tokenList.pushBack(std::move(token)))
		goto outOfMemory;
}

	return {};

outOfMemory:
	return LexicalError{ SourceLocation{ moduleNode, { 0, 0 }, { 0, 0 } }, LexicalErrorKind::OutOfMemory };
}
