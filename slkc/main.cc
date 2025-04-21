#include "comp/compiler.h"
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

typedef bool (*ArglessOptionCallback)(const OptionMatchContext &matchContext, const char *option);
typedef bool (*SingleArgOptionCallback)(const OptionMatchContext &matchContext, const char *option, const char *arg);
typedef bool (*CustomOptionCallback)(OptionMatchContext &matchContext, const char *option);
typedef bool (*FallbackOptionCallback)(OptionMatchContext &matchContext, const char *option);
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

[[nodiscard]] bool matchArgs(const CompiledOptionMap &optionMap, int argc, char **argv, void *userData) {
	OptionMatchContext matchContext = { argc, argv, 0, userData };
	for (int i = 1; i < argc; ++i) {
		if (auto it = optionMap.arglessOptions.find(std::string_view(argv[i])); it != optionMap.arglessOptions.end()) {
			if (!it.value()->callback(matchContext, argv[i])) {
				return false;
			}

			continue;
		}

		if (auto it = optionMap.singleArgOptions.find(std::string_view(argv[i])); it != optionMap.singleArgOptions.end()) {
			const char *opt = argv[i];
			if (++i == argc) {
				optionMap.requireOptionArgCallback(matchContext, *it.value());
				return false;
			}

			if (!it.value()->callback(matchContext, opt, argv[i])) {
				return false;
			}

			continue;
		}

		if (auto it = optionMap.customOptions.find(std::string_view(argv[i])); it != optionMap.customOptions.end()) {
			if (!it.value()->callback(matchContext, argv[i])) {
				return false;
			}

			continue;
		}

		if (!optionMap.fallbackOptionCallback(matchContext, argv[i])) {
			break;
		}
	}

	return true;
}

#define printError(fmt, ...) fprintf(stderr, "Error: " fmt, ##__VA_ARGS__)

const ArglessOptionMap g_arglessOptions = {

};

const SingleArgOptionMap g_singleArgOptions = {

};

const CustomOptionMap g_customOptions = {

};

const char *g_modFileName = nullptr;

void dumpLexicalError(const slkc::LexicalError &lexicalError) {
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

void dumpSyntaxError(const slkc::Parser &parser, const slkc::SyntaxError &syntaxError) {
	const slkc::Token *beginToken = parser.tokenList.at(syntaxError.tokenRange.beginIndex).get();
	const slkc::Token *endToken = parser.tokenList.at(syntaxError.tokenRange.endIndex).get();

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

void dumpCompilationError(const slkc::Parser &parser, const slkc::CompilationError &error) {
	const slkc::Token *beginToken = parser.tokenList.at(error.tokenRange.beginIndex).get();
	const slkc::Token *endToken = parser.tokenList.at(error.tokenRange.endIndex).get();

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
		case slkc::CompilationErrorKind::CyclicInheritedClass:
			printError("Error at %zu, %zu: Cyclic inheritance detected\n",
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
		case slkc::CompilationErrorKind::ImportLimitExceeded:
			printError("Error at %zu, %zu: Import item number exceeded\n",
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

int main(int argc, char *argv[]) {
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	CompiledOptionMap optionMap(
		peff::getDefaultAlloc(),
		[](OptionMatchContext &matchContext, const char *option) -> bool {
			if (g_modFileName) {
				printError("Duplicated target file name");
				return false;
			}

			g_modFileName = option;

			return true;
		},
		[](const OptionMatchContext &matchContext, const SingleArgOption &option) {
			printError("Option `%s' requires more arguments", option.name);
		});

	if (!buildOptionMap(optionMap, g_arglessOptions, g_singleArgOptions, g_customOptions)) {
		printError("Out of memory");
		return ENOMEM;
	}

	if (!matchArgs(optionMap, argc, argv, nullptr)) {
		return EINVAL;
	}

	if (!g_modFileName) {
		printError("Missing target file name");
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

		peff::Uninitialized<slkc::TokenList> tokenList;
		{
			slkc::Lexer lexer(peff::getDefaultAlloc());

			std::string_view sv(buf.get(), fileSize);

			if (auto e = lexer.lex(sv, peff::getDefaultAlloc(), document); e) {
				dumpLexicalError(*e);
				return -1;
			}

			tokenList.moveFrom(std::move(lexer.tokenList));
		}

		slake::Runtime runtime(peff::getDefaultAlloc());
		slkc::CompileContext compileContext(&runtime, document, &peff::g_nullAlloc, peff::getDefaultAlloc());
		{
			peff::NullAlloc nullAlloc;
			slkc::Parser parser(document, tokenList.release(), &nullAlloc, peff::getDefaultAlloc());

			peff::SharedPtr<slkc::ModuleNode> mod(peff::makeShared<slkc::ModuleNode>(peff::getDefaultAlloc(), peff::getDefaultAlloc(), document));
			document->rootModule = mod;

			if (auto e = parser.parseProgram(mod); e) {
				dumpSyntaxError(parser, *e);
			}

			for (auto &i : parser.syntaxErrors) {
				dumpSyntaxError(parser, i);
			}

			slake::HostObjectRef<slake::ModuleObject> modObj = slake::ModuleObject::alloc(&runtime, slake::ScopeUniquePtr(slake::Scope::alloc(&runtime.globalHeapPoolAlloc, runtime.getRootObject())), slake::ACCESS_PUB | slake::ACCESS_STATIC);

			if (auto e = slkc::compileModule(&compileContext, mod, modObj.get()); e) {
				dumpCompilationError(parser, *e);
			}

			std::sort(compileContext.errors.data(), compileContext.errors.data() + compileContext.errors.size());

			for (auto &i : compileContext.errors) {
				dumpCompilationError(parser, i);
			}
		}
	}

	return 0;
}
