#include <slkc/compiler/ast/lexer.h>
#include <algorithm>

using namespace slake::slkc;

enum LexCondition {
	yycInitialCondition = 0,

	yycStringCondition,
	yycEscapeCondition,

	yycCommentCondition,
	yycLineCommentCondition,
};

void slake::slkc::Lexer::lex(std::string_view src) {
	const char *YYCURSOR = src.data(), *YYMARKER = YYCURSOR, *YYLIMIT = src.data() + src.size();
	const char *prevYYCURSOR = YYCURSOR;

	LexCondition YYCONDITION = yycInitialCondition;

#define YYSETCONDITION(cond) (YYCONDITION = (yyc##cond))
#define YYGETCONDITION() (YYCONDITION)

	Token token;

	while (true) {
		std::string strLiteral;

		while (true) {
			/*!re2c
				re2c:yyfill:enable = 0;
				re2c:define:YYCTYPE = char;

				<InitialCondition>"/*"		{ YYSETCONDITION(CommentCondition); token.tokenId = TokenId::Comment; continue; }
				<InitialCondition>"//"		{ YYSETCONDITION(LineCommentCondition); token.tokenId = TokenId::Comment; continue; }

				<InitialCondition>"::"		{ token.tokenId = TokenId::ScopeOp; break; }
				<InitialCondition>"->"		{ token.tokenId = TokenId::WrapOp; break; }
				<InitialCondition>"=>"		{ token.tokenId = TokenId::MatchOp; break; }
				<InitialCondition>"&&"		{ token.tokenId = TokenId::LAndOp; break; }
				<InitialCondition>"||"		{ token.tokenId = TokenId::LOrOp; break; }
				<InitialCondition>"++"		{ token.tokenId = TokenId::IncOp; break; }
				<InitialCondition>"--"		{ token.tokenId = TokenId::DecOp; break; }
				<InitialCondition>"+"		{ token.tokenId = TokenId::AddOp; break; }
				<InitialCondition>"-"		{ token.tokenId = TokenId::SubOp; break; }
				<InitialCondition>"*"		{ token.tokenId = TokenId::MulOp; break; }
				<InitialCondition>"/"		{ token.tokenId = TokenId::DivOp; break; }
				<InitialCondition>"%"		{ token.tokenId = TokenId::ModOp; break; }
				<InitialCondition>"&"		{ token.tokenId = TokenId::AndOp; break; }
				<InitialCondition>"|"		{ token.tokenId = TokenId::OrOp; break; }
				<InitialCondition>"^"		{ token.tokenId = TokenId::XorOp; break; }
				<InitialCondition>"!"		{ token.tokenId = TokenId::LNotOp; break; }
				<InitialCondition>"~"		{ token.tokenId = TokenId::NotOp; break; }
				<InitialCondition>"="		{ token.tokenId = TokenId::AssignOp; break; }
				<InitialCondition>"+="		{ token.tokenId = TokenId::AddAssignOp; break; }
				<InitialCondition>"-="		{ token.tokenId = TokenId::SubAssignOp; break; }
				<InitialCondition>"*="		{ token.tokenId = TokenId::MulAssignOp; break; }
				<InitialCondition>"/="		{ token.tokenId = TokenId::DivAssignOp; break; }
				<InitialCondition>"%="		{ token.tokenId = TokenId::ModAssignOp; break; }
				<InitialCondition>"&="		{ token.tokenId = TokenId::AndAssignOp; break; }
				<InitialCondition>"|="		{ token.tokenId = TokenId::OrAssignOp; break; }
				<InitialCondition>"^="		{ token.tokenId = TokenId::XorAssignOp; break; }
				<InitialCondition>"~="		{ token.tokenId = TokenId::NotAssignOp; break; }
				<InitialCondition>"<<="		{ token.tokenId = TokenId::LshAssignOp; break; }
				<InitialCondition>">>="		{ token.tokenId = TokenId::RshAssignOp; break; }
				<InitialCondition>"==="		{ token.tokenId = TokenId::StrictEqOp; break; }
				<InitialCondition>"!=="		{ token.tokenId = TokenId::StrictNeqOp; break; }
				<InitialCondition>"=="		{ token.tokenId = TokenId::EqOp; break; }
				<InitialCondition>"!="		{ token.tokenId = TokenId::NeqOp; break; }
				<InitialCondition>"<<"		{ token.tokenId = TokenId::LshOp; break; }
				<InitialCondition>">>"		{ token.tokenId = TokenId::RshOp; break; }
				<InitialCondition>"<="		{ token.tokenId = TokenId::LtEqOp; break; }
				<InitialCondition>">="		{ token.tokenId = TokenId::GtEqOp; break; }
				<InitialCondition>"<"		{ token.tokenId = TokenId::LtOp; break; }
				<InitialCondition>">"		{ token.tokenId = TokenId::GtOp; break; }
				<InitialCondition>"$"		{ token.tokenId = TokenId::DollarOp; break; }

				<InitialCondition>"as"			{ token.tokenId = TokenId::AsKeyword; break; }
				<InitialCondition>"async"		{ token.tokenId = TokenId::AsyncKeyword; break; }
				<InitialCondition>"await"		{ token.tokenId = TokenId::AwaitKeyword; break; }
				<InitialCondition>"base"		{ token.tokenId = TokenId::BaseKeyword; break; }
				<InitialCondition>"break"		{ token.tokenId = TokenId::BreakKeyword; break; }
				<InitialCondition>"case"		{ token.tokenId = TokenId::CaseKeyword; break; }
				<InitialCondition>"catch"		{ token.tokenId = TokenId::CatchKeyword; break; }
				<InitialCondition>"class"		{ token.tokenId = TokenId::ClassKeyword; break; }
				<InitialCondition>"const"		{ token.tokenId = TokenId::ConstKeyword; break; }
				<InitialCondition>"continue"	{ token.tokenId = TokenId::ContinueKeyword; break; }
				<InitialCondition>"delete"		{ token.tokenId = TokenId::DeleteKeyword; break; }
				<InitialCondition>"default"		{ token.tokenId = TokenId::DefaultKeyword; break; }
				<InitialCondition>"else"		{ token.tokenId = TokenId::ElseKeyword; break; }
				<InitialCondition>"enum"		{ token.tokenId = TokenId::EnumKeyword; break; }
				<InitialCondition>"false"		{ token.tokenId = TokenId::FalseKeyword; break; }
				<InitialCondition>"fn"			{ token.tokenId = TokenId::FnKeyword; break; }
				<InitialCondition>"for"			{ token.tokenId = TokenId::ForKeyword; break; }
				<InitialCondition>"final"		{ token.tokenId = TokenId::FinalKeyword; break; }
				<InitialCondition>"if"			{ token.tokenId = TokenId::IfKeyword; break; }
				<InitialCondition>"import"		{ token.tokenId = TokenId::ImportKeyword; break; }
				<InitialCondition>"let"			{ token.tokenId = TokenId::LetKeyword; break; }
				<InitialCondition>"module"		{ token.tokenId = TokenId::ModuleKeyword; break; }
				<InitialCondition>"native"		{ token.tokenId = TokenId::NativeKeyword; break; }
				<InitialCondition>"new"			{ token.tokenId = TokenId::NewKeyword; break; }
				<InitialCondition>"null"		{ token.tokenId = TokenId::NullKeyword; break; }
				<InitialCondition>"override"	{ token.tokenId = TokenId::OverrideKeyword; break; }
				<InitialCondition>"operator"	{ token.tokenId = TokenId::OperatorKeyword; break; }
				<InitialCondition>"pub"			{ token.tokenId = TokenId::PubKeyword; break; }
				<InitialCondition>"return"		{ token.tokenId = TokenId::ReturnKeyword; break; }
				<InitialCondition>"static"		{ token.tokenId = TokenId::StaticKeyword; break; }
				<InitialCondition>"struct"		{ token.tokenId = TokenId::StructKeyword; break; }
				<InitialCondition>"switch"		{ token.tokenId = TokenId::SwitchKeyword; break; }
				<InitialCondition>"this"		{ token.tokenId = TokenId::ThisKeyword; break; }
				<InitialCondition>"throw"		{ token.tokenId = TokenId::ThrowKeyword; break; }
				<InitialCondition>"trait"		{ token.tokenId = TokenId::TraitKeyword; break; }
				<InitialCondition>"typeof"		{ token.tokenId = TokenId::TypeofKeyword; break; }
				<InitialCondition>"interface"	{ token.tokenId = TokenId::InterfaceKeyword; break; }
				<InitialCondition>"true"		{ token.tokenId = TokenId::TrueKeyword; break; }
				<InitialCondition>"try"			{ token.tokenId = TokenId::TryKeyword; break; }
				<InitialCondition>"use"			{ token.tokenId = TokenId::UseKeyword; break; }
				<InitialCondition>"while"		{ token.tokenId = TokenId::WhileKeyword; break; }
				<InitialCondition>"yield"		{ token.tokenId = TokenId::YieldKeyword; break; }

				<InitialCondition>"i8"			{ token.tokenId = TokenId::I8TypeName; break; }
				<InitialCondition>"i16"			{ token.tokenId = TokenId::I16TypeName; break; }
				<InitialCondition>"i32"			{ token.tokenId = TokenId::I32TypeName; break; }
				<InitialCondition>"i64"			{ token.tokenId = TokenId::I64TypeName; break; }
				<InitialCondition>"u8"			{ token.tokenId = TokenId::U8TypeName; break; }
				<InitialCondition>"u16"			{ token.tokenId = TokenId::U16TypeName; break; }
				<InitialCondition>"u32"			{ token.tokenId = TokenId::U32TypeName; break; }
				<InitialCondition>"u64"			{ token.tokenId = TokenId::U64TypeName; break; }
				<InitialCondition>"f32"			{ token.tokenId = TokenId::F32TypeName; break; }
				<InitialCondition>"f64"			{ token.tokenId = TokenId::F64TypeName; break; }
				<InitialCondition>"string"		{ token.tokenId = TokenId::StringTypeName; break; }
				<InitialCondition>"bool"		{ token.tokenId = TokenId::BoolTypeName; break; }
				<InitialCondition>"auto"		{ token.tokenId = TokenId::AutoTypeName; break; }
				<InitialCondition>"void"		{ token.tokenId = TokenId::VoidTypeName; break; }
				<InitialCondition>"any"			{ token.tokenId = TokenId::AnyTypeName; break; }

				<InitialCondition>","		{ token.tokenId = TokenId::Comma; break; }
				<InitialCondition>"?"		{ token.tokenId = TokenId::Question; break; }
				<InitialCondition>":"		{ token.tokenId = TokenId::Colon; break; }
				<InitialCondition>";"     	{ token.tokenId = TokenId::Semicolon; break; }
				<InitialCondition>"["		{ token.tokenId = TokenId::LBracket; break; }
				<InitialCondition>"]"		{ token.tokenId = TokenId::RBracket; break; }
				<InitialCondition>"{"		{ token.tokenId = TokenId::LBrace; break; }
				<InitialCondition>"}"		{ token.tokenId = TokenId::RBrace; break; }
				<InitialCondition>"("		{ token.tokenId = TokenId::LParenthese; break; }
				<InitialCondition>")"		{ token.tokenId = TokenId::RParenthese; break; }
				<InitialCondition>"..."		{ token.tokenId = TokenId::VarArg; break; }
				<InitialCondition>"."		{ token.tokenId = TokenId::Dot; break; }

				<InitialCondition>[a-zA-Z_][a-zA-Z0-9_]* {
					token.tokenId = TokenId::Id;
					break;
				}

				<InitialCondition>"0"[0-7]+ {
					token.tokenId = TokenId::UIntLiteral;
					token.exData = std::make_unique<UIntLiteralTokenExtension>(strtoul(prevYYCURSOR, nullptr, 8));
					break;
				}

				<InitialCondition>[0-9]+ {
					token.tokenId = TokenId::IntLiteral;
					token.exData = std::make_unique<IntLiteralTokenExtension>(strtol(prevYYCURSOR, nullptr, 10));
					break;
				}

				<InitialCondition>"0"[xX][0-9a-fA-F]+ {
					token.tokenId = TokenId::UIntLiteral;
					token.exData = std::make_unique<UIntLiteralTokenExtension>(strtoul(prevYYCURSOR, nullptr, 16));
					break;
				}

				<InitialCondition>"0"[bB][01]+ {
					token.tokenId = TokenId::UIntLiteral;
					token.exData = std::make_unique<UIntLiteralTokenExtension>(strtoul(prevYYCURSOR, nullptr, 2));
					break;
				}

				<InitialCondition>[0-9]+"."[0-9]+[fF] {
					token.tokenId = TokenId::F32Literal;
					token.exData = std::make_unique<F32LiteralTokenExtension>(strtof(prevYYCURSOR, nullptr));
					break;
				}

				<InitialCondition>[0-9]+"."[0-9]+ {
					token.tokenId = TokenId::F64Literal;
					token.exData = std::make_unique<F64LiteralTokenExtension>(strtod(prevYYCURSOR, nullptr));
					break;
				}

				<InitialCondition>"\""		{ YYSETCONDITION(StringCondition); continue; }

				<InitialCondition>"\n"		{ token.tokenId = TokenId::NewLine; break; }
				<InitialCondition>"\000"	{ goto end; }

				<InitialCondition>[ \r\t]+	{ token.tokenId = TokenId::Whitespace; break; }

				<InitialCondition>[^]		{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t index = prevYYCURSOR - src.data();
					auto pos = src.find_last_of('\n', index);
					if(pos == std::string::npos)
						pos = 0;
					pos = index - pos;

					throw LexicalError("Invalid token", { (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos });
				}

				<StringCondition>"\""		{
					YYSETCONDITION(InitialCondition);
					token.tokenId = TokenId::StringLiteral;
					token.exData = std::make_unique<StringLiteralTokenExtension>(strLiteral);
					strLiteral.clear();
					break;
				}
				<StringCondition>"\\\n"		{ continue; }
				<StringCondition>"\\"		{ YYSETCONDITION(EscapeCondition); continue; }
				<StringCondition>"\n"		{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t index = prevYYCURSOR - src.data();
					auto pos = src.find_last_of('\n', index);
					if(pos == std::string::npos)
						pos = 0;
					pos = index - pos;

					throw LexicalError("Unexpected end of line", { (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos });
				}
				<StringCondition>"\000"	{
					size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

					size_t index = prevYYCURSOR - src.data();
					auto pos = src.find_last_of('\n', index);
					if(pos == std::string::npos)
						pos = 0;
					pos = index - pos;

					throw LexicalError("Prematured end of file", { (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos });
				}
				<StringCondition>[^]		{ strLiteral += YYCURSOR[-1]; continue; }

				<EscapeCondition>"\'"	{ YYSETCONDITION(StringCondition); strLiteral += "\'"; continue; }
				<EscapeCondition>"\""	{ YYSETCONDITION(StringCondition); strLiteral += "\""; continue; }
				<EscapeCondition>"\?"	{ YYSETCONDITION(StringCondition); strLiteral += "\?"; continue; }
				<EscapeCondition>"\\"	{ YYSETCONDITION(StringCondition); strLiteral += "\\"; continue; }
				<EscapeCondition>"a"	{ YYSETCONDITION(StringCondition); strLiteral += "\a"; continue; }
				<EscapeCondition>"b"	{ YYSETCONDITION(StringCondition); strLiteral += "\b"; continue; }
				<EscapeCondition>"f"	{ YYSETCONDITION(StringCondition); strLiteral += "\f"; continue; }
				<EscapeCondition>"n"	{ YYSETCONDITION(StringCondition); strLiteral += "\n"; continue; }
				<EscapeCondition>"r"	{ YYSETCONDITION(StringCondition); strLiteral += "\r"; continue; }
				<EscapeCondition>"t"	{ YYSETCONDITION(StringCondition); strLiteral += "\t"; continue; }
				<EscapeCondition>"v"	{ YYSETCONDITION(StringCondition); strLiteral += "\v"; continue; }
				<EscapeCondition>[0-7]{1,3}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prevYYCURSOR;

					char c = 0;
					for(uint_fast8_t i = 0; i < size; ++i) {
						c *= 8;
						c += prevYYCURSOR[i] - '0';
					}

					strLiteral += "\0";
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

					strLiteral += "\0";
				}

				<CommentCondition>"*"[/]	{ YYSETCONDITION(InitialCondition); break; }
				<CommentCondition>[^]		{ continue; }

				<LineCommentCondition>"\n"	{ YYSETCONDITION(InitialCondition); break; }
				<CommentCondition>[^]		{ continue; }
			*/
		}

		size_t beginIndex = prevYYCURSOR - src.data(), endIndex = YYCURSOR - src.data();

		std::string_view strToBegin = src.substr(0, beginIndex), strToEnd = src.substr(0, endIndex);

		token.text = std::string(prevYYCURSOR, YYCURSOR - prevYYCURSOR);

		size_t idxLastBeginNewline = src.find_last_of('\n', beginIndex),
			   idxLastEndNewline = src.find_last_of('\n', endIndex);

		token.beginLocation = {
			(size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'),
			(idxLastBeginNewline == string::npos
					? beginIndex
					: beginIndex - idxLastBeginNewline - 1)
		};
		token.endLocation = {
			(size_t)std::count(strToEnd.begin(), strToEnd.end(), '\n'),
			(idxLastEndNewline == string::npos
					? endIndex
					: endIndex - idxLastEndNewline - 1)
		};
		tokens.push_back(std::move(token));

		prevYYCURSOR = YYCURSOR;
	}

end:

	_endToken = {
		TokenId::End,
		token.endLocation,
		token.endLocation,
		""
	};
}
