#include "comp/compiler.h"
#include "decomp/decompiler.h"
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

struct OptionMatchContext {
	const int argc;
	char **const argv;
	int i;
	void *userData;
};

struct SingleArgOption;

typedef int (*ArglessOptionCallback)(const OptionMatchContext &matchContext, const char *option);
typedef int (*SingleArgOptionCallback)(const OptionMatchContext &matchContext, const char *option, const char *arg);
typedef int (*CustomOptionCallback)(OptionMatchContext &matchContext, const char *option);
typedef int (*FallbackOptionCallback)(OptionMatchContext &matchContext, const char *option);
typedef void (*RequireOptionArgCallback)(const OptionMatchContext &matchContext, const SingleArgOption &option);

struct ArglessOption {
	const char *name;
	ArglessOptionCallback callback;
};

struct SingleArgOption {
	const char *name;
	SingleArgOptionCallback callback;
};

struct CustomOption {
	const char *name;
	CustomOptionCallback callback;
};

using ArglessOptionMap = std::initializer_list<ArglessOption>;
using SingleArgOptionMap = std::initializer_list<SingleArgOption>;
using CustomOptionMap = std::initializer_list<CustomOption>;

struct CompiledOptionMap {
	peff::HashMap<std::string_view, const ArglessOption *> arglessOptions;
	peff::HashMap<std::string_view, const SingleArgOption *> singleArgOptions;
	peff::HashMap<std::string_view, const CustomOption *> customOptions;
	FallbackOptionCallback fallbackOptionCallback;
	RequireOptionArgCallback requireOptionArgCallback;

	SLAKE_FORCEINLINE CompiledOptionMap(peff::Alloc *alloc, FallbackOptionCallback fallbackOptionCallback, RequireOptionArgCallback requireOptionArgCallback) noexcept : arglessOptions(alloc), singleArgOptions(alloc), customOptions(alloc), fallbackOptionCallback(fallbackOptionCallback), requireOptionArgCallback(requireOptionArgCallback) {}
};

[[nodiscard]] bool buildOptionMap(
	CompiledOptionMap &optionMapOut,
	const ArglessOptionMap &arglessOptions,
	const SingleArgOptionMap &singleArgOptions,
	const CustomOptionMap &customOptions) {
	for (const auto &i : arglessOptions) {
		if (!optionMapOut.arglessOptions.insert(std::string_view(i.name), &i)) {
			return false;
		}
	}

	for (const auto &i : singleArgOptions) {
		if (!optionMapOut.singleArgOptions.insert(std::string_view(i.name), &i)) {
			return false;
		}
	}

	for (const auto &i : customOptions) {
		if (!optionMapOut.customOptions.insert(std::string_view(i.name), &i)) {
			return false;
		}
	}

	return true;
}

[[nodiscard]] int matchArgs(const CompiledOptionMap &optionMap, int argc, char **argv, void *userData) {
	OptionMatchContext matchContext = { argc, argv, 0, userData };
	for (int i = 1; i < argc; ++i) {
		if (auto it = optionMap.arglessOptions.find(std::string_view(argv[i])); it != optionMap.arglessOptions.end()) {
			if (int result = it.value()->callback(matchContext, argv[i]); result) {
				return result;
			}

			continue;
		}

		if (auto it = optionMap.singleArgOptions.find(std::string_view(argv[i])); it != optionMap.singleArgOptions.end()) {
			const char *opt = argv[i];
			if (++i == argc) {
				optionMap.requireOptionArgCallback(matchContext, *it.value());
				return EINVAL;
			}

			if (int result = it.value()->callback(matchContext, opt, argv[i]); result) {
				return result;
			}

			continue;
		}

		if (auto it = optionMap.customOptions.find(std::string_view(argv[i])); it != optionMap.customOptions.end()) {
			if (int result = it.value()->callback(matchContext, argv[i]); result) {
				return result;
			}

			continue;
		}

		if (int result = optionMap.fallbackOptionCallback(matchContext, argv[i]); result) {
			return result;
		}
	}

	return 0;
}

#define printError(fmt, ...) fprintf(stderr, "Error: " fmt, ##__VA_ARGS__)

struct MatchUserData {
	peff::DynArray<peff::String> *includeDirs;
};

const ArglessOptionMap g_arglessOptions = {

};

const char *g_modFileName = nullptr, *g_outputFileName = nullptr;

const SingleArgOptionMap g_singleArgOptions = {
	{ "-I", [](const OptionMatchContext &matchContext, const char *option, const char *arg) -> int {
		 MatchUserData *userData = ((MatchUserData *)matchContext.userData);

		 peff::String dir(peff::getDefaultAlloc());

		 if (!dir.build(arg)) {
			 printError("Out of memory");
			 return ENOMEM;
		 }

		 if (!userData->includeDirs->pushBack(std::move(dir))) {
			 printError("Out of memory");
			 return ENOMEM;
		 }

		 return 0;
	 } },
	{ "-CTS", [](const OptionMatchContext &matchContext, const char *option, const char *arg) -> int {
		 MatchUserData *userData = ((MatchUserData *)matchContext.userData);

		 std::string_view s(arg);

		 size_t size = 0;
		 bool encounteredUnit = false;

		 for (size_t i = 0; i < s.size(); ++i) {
			 switch (s[i]) {
				 case '0':
				 case '1':
				 case '2':
				 case '3':
				 case '4':
				 case '5':
				 case '6':
				 case '7':
				 case '8':
				 case '9':
					 if (encounteredUnit) {
						 printError("Invalid stack size");
						 return EINVAL;
					 }
					 if (size >= SIZE_MAX / 10) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size += s[i] - '0';
					 break;
				 case 'K':
					 if (size >= SIZE_MAX / 1024) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024;
					 encounteredUnit = true;
					 break;
				 case 'M':
					 if (size >= SIZE_MAX / 1024 / 1024) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024;
					 encounteredUnit = true;
					 break;
				 case 'G':
					 if (size >= SIZE_MAX / 1024 / 1024 / 1024) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024 * 1024;
					 encounteredUnit = true;
					 break;
				 default:
					 printError("Invalid stack size");
					 return EINVAL;
			 }
		 }

		 if (!size) {
			 printError("Invalid stack size");
			 return EINVAL;
		 }

		 slkc::szDefaultCompileThreadStack = size;

		 return 0;
	 } },
	{ "-PTS", [](const OptionMatchContext &matchContext, const char *option, const char *arg) -> int {
		 MatchUserData *userData = ((MatchUserData *)matchContext.userData);

		 std::string_view s(arg);

		 size_t size = 0;
		 bool encounteredUnit = false;

		 for (size_t i = 0; i < s.size(); ++i) {
			 switch (s[i]) {
				 case '0':
				 case '1':
				 case '2':
				 case '3':
				 case '4':
				 case '5':
				 case '6':
				 case '7':
				 case '8':
				 case '9':
					 if (encounteredUnit) {
						 printError("Invalid stack size");
						 return EINVAL;
					 }
					 if (size >= SIZE_MAX / 10) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size += s[i] - '0';
					 break;
				 case 'K':
					 if (size >= SIZE_MAX / 1024) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024;
					 encounteredUnit = true;
					 break;
				 case 'M':
					 if (size >= SIZE_MAX / 1024 / 1024) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024;
					 encounteredUnit = true;
					 break;
				 case 'G':
					 if (size >= SIZE_MAX / 1024 / 1024 / 1024) {
						 printError("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024 * 1024;
					 encounteredUnit = true;
					 break;
				 default:
					 printError("Invalid stack size");
					 return EINVAL;
			 }
		 }

		 if (!size) {
			 printError("Invalid stack size");
			 return EINVAL;
		 }

		 slkc::szDefaultParseThreadStack = size;

		 return 0;
	 } },
	{ "-o", [](const OptionMatchContext &matchContext, const char *option, const char *arg) -> int {
		 g_outputFileName = arg;

		 return 0;
	 } }
};

const CustomOptionMap g_customOptions = {

};

void dumpLexicalError(const slkc::LexicalError &lexicalError, int indentLevel = 0) {
	for (int i = 0; i < indentLevel; ++i) {
		putc('\t', stderr);
	}

	switch (lexicalError.kind) {
		case slkc::LexicalErrorKind::UnrecognizedToken:
			printError("Syntax error at %zu, %zu: Unrecognized token\n",
				lexicalError.location.beginPosition.line + 1,
				lexicalError.location.beginPosition.column + 1);
			break;
		case slkc::LexicalErrorKind::UnexpectedEndOfLine:
			printError("Syntax error at %zu, %zu: Unexpected end of line\n",
				lexicalError.location.beginPosition.line + 1,
				lexicalError.location.beginPosition.column + 1);
			break;
		case slkc::LexicalErrorKind::PrematuredEndOfFile:
			printError("Syntax error at %zu, %zu: Prematured end of file\n",
				lexicalError.location.beginPosition.line + 1,
				lexicalError.location.beginPosition.column + 1);
			break;
		case slkc::LexicalErrorKind::OutOfMemory:
			printError("Out of memory during lexical analysis\n");
			break;
	}
}

void dumpSyntaxError(slkc::Parser *parser, const slkc::SyntaxError &syntaxError, int indentLevel = 0) {
	const slkc::Token *beginToken = parser->tokenList.at(syntaxError.tokenRange.beginIndex).get();
	const slkc::Token *endToken = parser->tokenList.at(syntaxError.tokenRange.endIndex).get();

	for (int i = 0; i < indentLevel; ++i) {
		putc('\t', stderr);
	}

	switch (syntaxError.errorKind) {
		case slkc::SyntaxErrorKind::OutOfMemory:
			printError("Syntax error at %zu, %zu: Out of memory\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::UnexpectedToken:
			printError("Syntax error at %zu, %zu: Unexpected token\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::ExpectingSingleToken:
			printError("Syntax error at %zu, %zu: Expecting %s\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1,
				slkc::getTokenName(std::get<slkc::ExpectingSingleTokenErrorExData>(syntaxError.exData).expectingTokenId));
			break;
		case slkc::SyntaxErrorKind::ExpectingTokens: {
			printError("Syntax error at %zu, %zu: Expecting ",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);

			const slkc::ExpectingTokensErrorExData &exData = std::get<slkc::ExpectingTokensErrorExData>(syntaxError.exData);

			auto it = exData.expectingTokenIds.begin();

			fprintf(stderr, "%s", slkc::getTokenName(*it));

			while (++it != exData.expectingTokenIds.end()) {
				fprintf(stderr, " or %s", slkc::getTokenName(*it));
			}

			fprintf(stderr, "\n");
			break;
		}
		case slkc::SyntaxErrorKind::ExpectingId:
			printError("Syntax error at %zu, %zu: Expecting an identifier\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::ExpectingExpr:
			printError("Syntax error at %zu, %zu: Expecting an expression\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::ExpectingStmt:
			printError("Syntax error at %zu, %zu: Expecting a statement\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::ExpectingDecl:
			printError("Syntax error at %zu, %zu: Expecting a declaration\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::NoMatchingTokensFound:
			printError("Syntax error at %zu, %zu: Matching token not found\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::SyntaxErrorKind::ConflictingDefinitions: {
			printError("Syntax error at %zu, %zu: Definition of `",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);

			const slkc::ConflictingDefinitionsErrorExData &exData = std::get<slkc::ConflictingDefinitionsErrorExData>(syntaxError.exData);

			fprintf(stderr, "%s' conflicts with other definitions\n", exData.memberName.data());
			break;
		}
		default:
			printError("Syntax error at %zu, %zu: Unknown error (%d)\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1,
				(int)syntaxError.errorKind);
			break;
	}
}

void dumpCompilationError(peff::SharedPtr<slkc::Parser> parser, const slkc::CompilationError &error, int indentLevel = 0) {
	const slkc::Token *beginToken = parser->tokenList.at(error.tokenRange.beginIndex).get();
	const slkc::Token *endToken = parser->tokenList.at(error.tokenRange.endIndex).get();

	for (int i = 0; i < indentLevel; ++i) {
		putc('\t', stderr);
	}

	switch (error.errorKind) {
		case slkc::CompilationErrorKind::OutOfMemory:
			printError("Error at %zu, %zu: Out of memory\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::OutOfRuntimeMemory:
			printError("Error at %zu, %zu: Slake runtime memory allocation limit exceeded\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ExpectingLValueExpr:
			printError("Error at %zu, %zu: Expecting a lvalue expression\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::TargetIsNotCallable:
			printError("Error at %zu, %zu: Expression is not callable\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::OperatorNotFound:
			printError("Error at %zu, %zu: No matching operator found\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::MismatchedGenericArgNumber:
			printError("Error at %zu, %zu: Mismatched generic argument number\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::DoesNotReferToATypeName:
			printError("Error at %zu, %zu: Expecting a type name\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ExpectingClassName:
			printError("Error at %zu, %zu: Expecting a class name\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ExpectingInterfaceName:
			printError("Error at %zu, %zu: Expecting an interface name\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::AbstractMethodNotImplemented:
			printError("Error at %zu, %zu: Abstract method is not implemented\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::CyclicInheritedClass:
			printError("Error at %zu, %zu: Cyclic inherited class detected\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::CyclicInheritedInterface:
			printError("Error at %zu, %zu: Cyclic inherited interface detected\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::RecursedStruct:
			printError("Error at %zu, %zu: Recursed structure type detected\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ExpectingId:
			printError("Error at %zu, %zu: Expecting an identifier\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::IdNotFound:
			printError("Error at %zu, %zu: Identifier not found\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InvalidThisUsage:
			printError("Error at %zu, %zu: Cannot use this keyword in this context\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::NoMatchingFnOverloading:
			printError("Error at %zu, %zu: No matching function overloading\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::UnableToDetermineOverloading:
			printError("Error at %zu, %zu: Unable to determine the overloading\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ArgsMismatched:
			printError("Error at %zu, %zu: Mismatched argument types\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::MemberAlreadyDefined:
			printError("Error at %zu, %zu: Member is already defined\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::MissingBindingObject:
			printError("Error at %zu, %zu: Missing binding target\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::RedundantWithObject:
			printError("Error at %zu, %zu: Redundant binding target\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ParamAlreadyDefined:
			printError("Error at %zu, %zu: Parameter is already defined\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::GenericParamAlreadyDefined:
			printError("Error at %zu, %zu: Generic parameter is already defined\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InvalidInitializerListUsage:
			printError("Error at %zu, %zu: Cannot use initializer list in this context\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorDeducingInitializerListType:
			printError("Error at %zu, %zu: Error deducing type of the initializer list\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorDeducingSwitchConditionType:
			printError("Error at %zu, %zu: Error deducing type of the switch condition\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorDeducingArgType:
			printError("Error at %zu, %zu: Error deducing type of the argument\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorEvaluatingConstSwitchCaseCondition:
			printError("Error at %zu, %zu: The switch condition is required to be a comptime evaluatable expression\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::MismatchedSwitchCaseConditionType:
			printError("Error at %zu, %zu: Mismatched switch condition type\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::DuplicatedSwitchCaseBranch:
			printError("Error at %zu, %zu: Duplicated switch case\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorDeducingMatchConditionType:
			printError("Error at %zu, %zu: Error deducing type of the match condition\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorDeducingMatchResultType:
			printError("Error at %zu, %zu: Error deducing return type of the match expression\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorEvaluatingConstMatchCaseCondition:
			printError("Error at %zu, %zu: The match condition is required to be a comptime evaluatable expression\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::MismatchedMatchCaseConditionType:
			printError("Error at %zu, %zu: Mismatched case condition type\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::DuplicatedMatchCaseBranch:
			printError("Error at %zu, %zu: Duplicated match case\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::LocalVarAlreadyExists:
			printError("Error at %zu, %zu: Local variable already exists\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InvalidBreakUsage:
			printError("Error at %zu, %zu: Cannot use break in this context\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InvalidContinueUsage:
			printError("Error at %zu, %zu: Cannot use continue in this context\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InvalidCaseLabelUsage:
			printError("Error at %zu, %zu: Cannot use case label in this context\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::TypeIsNotConstructible:
			printError("Error at %zu, %zu: Type is not constructible\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InvalidCast:
			printError("Error at %zu, %zu: Invalid type cast\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::FunctionOverloadingDuplicated:
			printError("Error at %zu, %zu: Duplicated function overloading\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::InterfaceMethodsConflicted:
			printError("Error at %zu, %zu: Interface methods conflicted\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ImportLimitExceeded:
			printError("Error at %zu, %zu: Import item number exceeded\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		case slkc::CompilationErrorKind::ErrorParsingImportedModule: {
			const slkc::ErrorParsingImportedModuleErrorExData &exData = std::get<slkc::ErrorParsingImportedModuleErrorExData>(error.exData);

			printError("Error at %zu, %zu: Error parsing imported module:\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			if (exData.lexicalError) {
				dumpLexicalError(*exData.lexicalError, indentLevel + 1);
			} else {
				for (auto &i : exData.mod->parser->syntaxErrors) {
					dumpSyntaxError(exData.mod->parser, i, indentLevel + 1);
				}
			}
			break;
		}
		case slkc::CompilationErrorKind::ModuleNotFound:
			printError("Error at %zu, %zu: Module not found\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1);
			break;
		default:
			printError("Error at %zu, %zu: Unknown error (%d)\n",
				beginToken->sourceLocation.beginPosition.line + 1,
				beginToken->sourceLocation.beginPosition.column + 1,
				(int)error.errorKind);
			break;
	}
}

class FileWriter : public slkc::Writer {
public:
	FILE *fp = NULL;

	FileWriter(FILE *fp) : fp(fp) {
	}

	SLKC_API virtual ~FileWriter() {
		if (fp)
			fclose(fp);
	}

	virtual peff::Option<slkc::CompilationError> write(const char *src, size_t size) override {
		if (fwrite(src, size, 1, fp) < 1) {
			return slkc::CompilationError(slkc::TokenRange{ nullptr, 0 }, slkc::CompilationErrorKind::ErrorWritingCompiledModule);
		}
		return {};
	}
};

class ANSIDumpWriter : public slkc::DumpWriter {
public:
	ANSIDumpWriter() {
	}

	SLKC_API virtual ~ANSIDumpWriter() {
	}

	virtual bool write(const char *src, size_t size) override {
		fwrite(src, size, 1, stdout);
		return true;
	}
};

int main(int argc, char *argv[]) {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	peff::DynArray<peff::String> includeDirs(peff::getDefaultAlloc());
	{
		CompiledOptionMap optionMap(
			peff::getDefaultAlloc(),
			[](OptionMatchContext &matchContext, const char *option) -> int {
				if (g_modFileName) {
					printError("Duplicated target file name");
					return EINVAL;
				}

				g_modFileName = option;

				return 0;
			},
			[](const OptionMatchContext &matchContext, const SingleArgOption &option) {
				printError("Option `%s' requires more arguments", option.name);
			});

		if (!buildOptionMap(optionMap, g_arglessOptions, g_singleArgOptions, g_customOptions)) {
			printError("Out of memory");
			return ENOMEM;
		}

		{
			MatchUserData matchUserData = {};
			matchUserData.includeDirs = &includeDirs;

			if (int result = matchArgs(optionMap, argc, argv, &matchUserData); result) {
				return result;
			}
		}
	}

	if (!g_modFileName) {
		printError("Missing target file name");
		return EINVAL;
	}

	if (!g_outputFileName) {
		printError("Missing output file name");
		return EINVAL;
	}

	FILE *fp = fopen(g_modFileName, "rb");

	if (!fp) {
		printError("Error opening the file");
		return EIO;
	}

	peff::ScopeGuard closeFpGuard([fp]() noexcept {
		fclose(fp);
	});

	if (fseek(fp, 0, SEEK_END)) {
		printError("Error evaluating file size");
		return EIO;
	}

	long fileSize;
	if ((fileSize = ftell(fp)) < 1) {
		printError("Error evaluating file size");
		return EIO;
	}

	if (fseek(fp, 0, SEEK_SET)) {
		printError("Error evaluating file size");
		return EIO;
	}

	{
		auto deleter = [](char *ptr) {
			free(ptr);
		};
		std::unique_ptr<char[], decltype(deleter)> buf((char *)malloc((size_t)fileSize + 1), deleter);

		if (!buf) {
			printError("Error allocating memory for reading the file");
			return ENOMEM;
		}

		(buf.get())[fileSize] = '\0';

		if (fread(buf.get(), fileSize, 1, fp) < 1) {
			printError("Error reading the file");
			return EIO;
		}

		peff::SharedPtr<slkc::Document> document(peff::makeShared<slkc::Document>(peff::getDefaultAlloc(), peff::getDefaultAlloc()));

		peff::SharedPtr<slkc::FileSystemExternalModuleProvider> fsExternalModProvider;

		if (!(fsExternalModProvider = peff::makeShared<slkc::FileSystemExternalModuleProvider>(peff::getDefaultAlloc(), peff::getDefaultAlloc()))) {
			printError("Out of memory");
			return ENOMEM;
		}

		for (auto &i : includeDirs) {
			if (!fsExternalModProvider->importPaths.pushBack(std::move(i))) {
				printError("Out of memory");
				return ENOMEM;
			}
		}

		includeDirs.clear();

		if (!document->externalModuleProviders.pushBack(fsExternalModProvider.template castTo<slkc::ExternalModuleProvider>())) {
			printError("Out of memory");
			return ENOMEM;
		}

		slkc::AstNodePtr<slkc::ModuleNode> mod(peff::makeSharedWithControlBlock<slkc::ModuleNode, slkc::AstNodeControlBlock<slkc::ModuleNode>>(peff::getDefaultAlloc(), peff::getDefaultAlloc(), document));
		if (!(mod = peff::makeSharedWithControlBlock<slkc::ModuleNode, slkc::AstNodeControlBlock<slkc::ModuleNode>>(peff::getDefaultAlloc(), peff::getDefaultAlloc(), document))) {
			printError("Error allocating memory for the target module");
			return ENOMEM;
		}

		document->mainModule = mod.get();

		peff::Uninitialized<slkc::TokenList> tokenList;
		{
			slkc::Lexer lexer(peff::getDefaultAlloc());

			std::string_view sv(buf.get(), fileSize);

			if (auto e = lexer.lex(mod.get(), sv, peff::getDefaultAlloc(), document); e) {
				dumpLexicalError(*e);
				return -1;
			}

			tokenList.moveFrom(std::move(lexer.tokenList));
		}

		std::unique_ptr<slake::Runtime, peff::DeallocableDeleter<slake::Runtime>> runtime(
			slake::Runtime::alloc(peff::getDefaultAlloc(), peff::getDefaultAlloc()));
		if (!runtime) {
			printError("Error allocating memory for the runtime");
			return ENOMEM;
		}
		slkc::CompileEnvironment compileEnv(runtime.get(), document, &peff::g_nullAlloc, peff::getDefaultAlloc());
		{
			peff::SharedPtr<slkc::Parser> parser;
			if (!(parser = peff::makeShared<slkc::Parser>(peff::getDefaultAlloc(), document, tokenList.release(), peff::getDefaultAlloc()))) {
				printError("Error allocating memory for the parser");
				return ENOMEM;
			}

			slkc::AstNodePtr<slkc::ModuleNode> rootMod;
			if (!(rootMod = peff::makeSharedWithControlBlock<slkc::ModuleNode, slkc::AstNodeControlBlock<slkc::ModuleNode>>(peff::getDefaultAlloc(), peff::getDefaultAlloc(), document))) {
				printError("Error allocating memory for the root module");
				return ENOMEM;
			}
			document->rootModule = rootMod;

			slkc::IdRefPtr moduleName;

			bool encounteredErrors = false;
			if (auto e = parser->parseProgram(mod, moduleName); e) {
				encounteredErrors = true;
				dumpSyntaxError(parser, *e);
			}

			for (auto &i : parser->syntaxErrors) {
				encounteredErrors = true;
				dumpSyntaxError(parser, i);
			}

			if (auto e = completeParentModules(&compileEnv, moduleName.get(), mod); e) {
				encounteredErrors = true;
				dumpCompilationError(parser, *e);
			}

			if (auto e = indexModuleMembers(&compileEnv, rootMod); e) {
				encounteredErrors = true;
				dumpCompilationError(parser, *e);
			}

			slake::HostObjectRef<slake::ModuleObject> modObj = slake::ModuleObject::alloc(runtime.get());
			modObj->setAccess(slake::ACCESS_PUBLIC | slake::ACCESS_STATIC);

			if (auto e = slkc::compileModule(&compileEnv, mod, modObj.get()); e) {
				encounteredErrors = true;
				dumpCompilationError(parser, *e);
			}

			// Sort errors in order.
			std::sort(compileEnv.errors.data(), compileEnv.errors.data() + compileEnv.errors.size());

			for (auto &i : compileEnv.errors) {
				encounteredErrors = true;
				dumpCompilationError(parser, i);
			}

			if (!encounteredErrors) {
				slake::HostObjectRef<slake::ModuleObject> lastModule = runtime->getRootObject();

				for (size_t i = 0; i < moduleName->entries.size() - 1; ++i) {
					slkc::IdRefEntry &e = moduleName->entries.at(i);

					if (auto curMod = lastModule->getMember(e.name); curMod) {
						lastModule = (slake::ModuleObject *)curMod.asObject;

						continue;
					}

					slake::HostObjectRef<slake::ModuleObject> curModule;

					if (!(curModule = slake::ModuleObject::alloc(runtime.get()))) {
						puts("Error dumping compiled module!");
					}

					if (!curModule->setName(e.name)) {
						puts("Error dumping compiled module!");
					}

					if (lastModule) {
						if (!lastModule->addMember(curModule.get())) {
							puts("Error dumping compiled module!");
						}
						curModule->setParent(lastModule.get());
					}

					lastModule = curModule;
				}

				if (!modObj->setName(moduleName->entries.back().name)) {
					puts("Error dumping compiled module!");
				}

				if (!lastModule->addMember(modObj.get())) {
					puts("Error dumping compiled module!");
				}
				modObj->setParent(lastModule.get());

				FILE *fp;

				if (!(fp = fopen(g_outputFileName, "wb"))) {
					printError("Error opening the output file");
				}
				FileWriter w(fp);
				if (auto e = slkc::dumpModule(peff::getDefaultAlloc(), &w, modObj.get())) {
					encounteredErrors = true;
					dumpCompilationError(parser, *e);
				}
			}

			ANSIDumpWriter dumpWriter;

			if (!slkc::decompileModule(peff::getDefaultAlloc(), &dumpWriter, modObj.get())) {
				puts("Error dumping compiled module!");
			}

			document->rootModule = {};
			document->genericCacheDir.clear();
			document->externalModuleProviders.clear();
		}
	}

	return 0;
}
