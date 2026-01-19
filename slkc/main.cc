#include "comp/compiler.h"
#include "decomp/decompiler.h"
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

#if SLKC_WITH_LANGUAGE_SERVER
	#include "server/server.h"
#endif

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

bool isBCMode = false;

const ArglessOptionMap g_arglessOptions = {
	{ "-bc", [](const OptionMatchContext &matchContext, const char *option) -> int {
		 isBCMode = true;
		 return 0;
	 } }
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

	printError("Syntax error at %zu, %zu: ",
		lexicalError.location.beginPosition.line + 1,
		lexicalError.location.beginPosition.column + 1);
	switch (lexicalError.kind) {
		case slkc::LexicalErrorKind::UnrecognizedToken:
			printf("Unrecognized token\n");
			break;
		case slkc::LexicalErrorKind::UnexpectedEndOfLine:
			printf("Unexpected end of line\n");
			break;
		case slkc::LexicalErrorKind::PrematuredEndOfFile:
			printf("Prematured end of file\n");
			break;
		case slkc::LexicalErrorKind::InvalidEscape:
			printf("Invalid escape sequence\n");
			break;
		case slkc::LexicalErrorKind::OutOfMemory:
			printf("Out of memory during lexical analysis\n");
			break;
	}
}

void dumpSyntaxError(slkc::Parser *parser, const slkc::SyntaxError &syntaxError, int indentLevel = 0) {
	const slkc::Token *beginToken = parser->tokenList.at(syntaxError.tokenRange.beginIndex).get();
	const slkc::Token *endToken = parser->tokenList.at(syntaxError.tokenRange.endIndex).get();

	for (int i = 0; i < indentLevel; ++i) {
		putc('\t', stderr);
	}

	size_t line = beginToken->sourceLocation.beginPosition.line + 1;
	size_t column = beginToken->sourceLocation.beginPosition.column + 1;

	printError("Syntax error at %zu, %zu: ", line, column);

	switch (syntaxError.errorKind) {
		case slkc::SyntaxErrorKind::OutOfMemory:
			printf("Out of memory\n");
			break;
		case slkc::SyntaxErrorKind::UnexpectedToken:
			printf("Unexpected token\n");
			break;
		case slkc::SyntaxErrorKind::ExpectingSingleToken:
			printf("Expecting %s\n",
				slkc::getTokenName(std::get<slkc::ExpectingSingleTokenErrorExData>(syntaxError.exData).expectingTokenId));
			break;
		case slkc::SyntaxErrorKind::ExpectingTokens: {
			printf("Expecting ");

			const slkc::ExpectingTokensErrorExData &exData = std::get<slkc::ExpectingTokensErrorExData>(syntaxError.exData);

			if (exData.expectingTokenIds.size()) {
				auto it = exData.expectingTokenIds.begin();

				fprintf(stderr, "%s", slkc::getTokenName(*it));

				while (++it != exData.expectingTokenIds.end()) {
					fprintf(stderr, " or %s", slkc::getTokenName(*it));
				}
			} else {
				fprintf(stderr, " token");
			}

			fprintf(stderr, "\n");
			break;
		}
		case slkc::SyntaxErrorKind::ExpectingId:
			printf("Expecting an identifier\n");
			break;
		case slkc::SyntaxErrorKind::ExpectingExpr:
			printf("Expecting an expression\n");
			break;
		case slkc::SyntaxErrorKind::ExpectingStmt:
			printf("Expecting a statement\n");
			break;
		case slkc::SyntaxErrorKind::ExpectingDecl:
			printf("Expecting a declaration\n");
			break;
		case slkc::SyntaxErrorKind::NoMatchingTokensFound:
			printf("Matching token not found\n");
			break;
		case slkc::SyntaxErrorKind::ConflictingDefinitions: {
			printf("Definition of `");

			const slkc::ConflictingDefinitionsErrorExData &exData = std::get<slkc::ConflictingDefinitionsErrorExData>(syntaxError.exData);

			fprintf(stderr, "%s' conflicts with other definitions\n", exData.memberName.data());
			break;
		}
		default:
			printf("Unknown error (%d)\n", (int)syntaxError.errorKind);
			break;
	}
}

void dumpCompilationError(peff::SharedPtr<slkc::Parser> parser, const slkc::CompilationError &error, int indentLevel = 0) {
	const slkc::Token *beginToken = parser->tokenList.at(error.tokenRange.beginIndex).get();
	const slkc::Token *endToken = parser->tokenList.at(error.tokenRange.endIndex).get();

	for (int i = 0; i < indentLevel; ++i) {
		putc('\t', stderr);
	}

	printError("Error at %zu, %zu to %zu, %zu: ",
		beginToken->sourceLocation.beginPosition.line + 1, beginToken->sourceLocation.beginPosition.column + 1,
		endToken->sourceLocation.endPosition.line + 1, endToken->sourceLocation.endPosition.column + 1);
	switch (error.errorKind) {
		case slkc::CompilationErrorKind::OutOfMemory:
			printf("Out of memory\n");
			break;
		case slkc::CompilationErrorKind::OutOfRuntimeMemory:
			printf("Slake runtime memory allocation limit exceeded\n");
			break;
		case slkc::CompilationErrorKind::ExpectingLValueExpr:
			printf("Expecting a lvalue expression\n");
			break;
		case slkc::CompilationErrorKind::TargetIsNotCallable:
			printf("Expression is not callable\n");
			break;
		case slkc::CompilationErrorKind::NoSuchFnOverloading:
			printf("No such function overloading\n");
			break;
		case slkc::CompilationErrorKind::IncompatibleOperand:
			printf("Incompatible operand\n");
			break;
		case slkc::CompilationErrorKind::OperatorNotFound:
			printf("No matching operator found\n");
			break;
		case slkc::CompilationErrorKind::MismatchedGenericArgNumber:
			printf("Mismatched generic argument number\n");
			break;
		case slkc::CompilationErrorKind::DoesNotReferToATypeName:
			printf("Expecting a type name\n");
			break;
		case slkc::CompilationErrorKind::ExpectingClassName:
			printf("Expecting a class name\n");
			break;
		case slkc::CompilationErrorKind::ExpectingInterfaceName:
			printf("Expecting an interface name\n");
			break;
		case slkc::CompilationErrorKind::AbstractMethodNotImplemented:
			printf("Abstract method is not implemented\n");
			break;
		case slkc::CompilationErrorKind::CyclicInheritedClass:
			printf("Cyclic inherited class detected\n");
			break;
		case slkc::CompilationErrorKind::CyclicInheritedInterface:
			printf("Cyclic inherited interface detected\n");
			break;
		case slkc::CompilationErrorKind::RecursedValueType:
			printf("Recursed value type detected\n");
			break;
		case slkc::CompilationErrorKind::ExpectingId:
			printf("Expecting an identifier\n");
			break;
		case slkc::CompilationErrorKind::IdNotFound:
			printf("Identifier not found\n");
			break;
		case slkc::CompilationErrorKind::InvalidThisUsage:
			printf("Cannot use this keyword in this context\n");
			break;
		case slkc::CompilationErrorKind::NoMatchingFnOverloading:
			printf("No matching function overloading\n");
			break;
		case slkc::CompilationErrorKind::UnableToDetermineOverloading:
			printf("Unable to determine the overloading\n");
			break;
		case slkc::CompilationErrorKind::ArgsMismatched:
			printf("Mismatched argument types\n");
			break;
		case slkc::CompilationErrorKind::MemberAlreadyDefined:
			printf("Member is already defined\n");
			break;
		case slkc::CompilationErrorKind::MissingBindingObject:
			printf("Missing binding target\n");
			break;
		case slkc::CompilationErrorKind::RedundantWithObject:
			printf("Redundant binding target\n");
			break;
		case slkc::CompilationErrorKind::ParamAlreadyDefined:
			printf("Parameter is already defined\n");
			break;
		case slkc::CompilationErrorKind::GenericParamAlreadyDefined:
			printf("Generic parameter is already defined\n");
			break;
		case slkc::CompilationErrorKind::InvalidInitializerListUsage:
			printf("Cannot use initializer list in this context\n");
			break;
		case slkc::CompilationErrorKind::ErrorDeducingInitializerListType:
			printf("Error deducing type of the initializer list\n");
			break;
		case slkc::CompilationErrorKind::ErrorDeducingSwitchConditionType:
			printf("Error deducing type of the switch condition\n");
			break;
		case slkc::CompilationErrorKind::ErrorDeducingArgType:
			printf("Error deducing type of the argument\n");
			break;
		case slkc::CompilationErrorKind::ErrorEvaluatingConstSwitchCaseCondition:
			printf("The switch condition is required to be a comptime evaluatable expression\n");
			break;
		case slkc::CompilationErrorKind::MismatchedSwitchCaseConditionType:
			printf("Mismatched switch condition type\n");
			break;
		case slkc::CompilationErrorKind::DuplicatedSwitchCaseBranch:
			printf("Duplicated switch case\n");
			break;
		case slkc::CompilationErrorKind::ErrorDeducingMatchConditionType:
			printf("Error deducing type of the match condition\n");
			break;
		case slkc::CompilationErrorKind::ErrorDeducingMatchResultType:
			printf("Error deducing return type of the match expression\n");
			break;
		case slkc::CompilationErrorKind::ErrorEvaluatingConstMatchCaseCondition:
			printf("The match condition is required to be a comptime evaluatable expression\n");
			break;
		case slkc::CompilationErrorKind::MismatchedMatchCaseConditionType:
			printf("Mismatched case condition type\n");
			break;
		case slkc::CompilationErrorKind::DuplicatedMatchCaseBranch:
			printf("Duplicated match case\n");
			break;
		case slkc::CompilationErrorKind::MissingDefaultMatchCaseBranch:
			printf("Missing default match case\n");
			break;
		case slkc::CompilationErrorKind::LocalVarAlreadyExists:
			printf("Local variable already exists\n");
			break;
		case slkc::CompilationErrorKind::InvalidBreakUsage:
			printf("Cannot use break in this context\n");
			break;
		case slkc::CompilationErrorKind::InvalidContinueUsage:
			printf("Cannot use continue in this context\n");
			break;
		case slkc::CompilationErrorKind::InvalidCaseLabelUsage:
			printf("Cannot use case label in this context\n");
			break;
		case slkc::CompilationErrorKind::TypeIsNotConstructible:
			printf("Type is not constructible\n");
			break;
		case slkc::CompilationErrorKind::InvalidCast:
			printf("Invalid type cast\n");
			break;
		case slkc::CompilationErrorKind::FunctionOverloadingDuplicated:
			printf("Duplicated function overloading\n");
			break;
		case slkc::CompilationErrorKind::RequiresInitialValue:
			printf("Requires an initial value\n");
			break;
		case slkc::CompilationErrorKind::ErrorDeducingVarType:
			printf("Error deducing the variable type\n");
			break;
		case slkc::CompilationErrorKind::TypeIsNotUnpackable:
			printf("Type is not unpackable\n");
			break;
		case slkc::CompilationErrorKind::InvalidVarArgHintDuringInstantiation:
			printf("Invalid variable argument hint during generic instantiation\n");
			break;
		case slkc::CompilationErrorKind::CannotBeUnpackedInThisContext:
			printf("Cannot be unpacked here\n");
			break;
		case slkc::CompilationErrorKind::TypeIsNotSubstitutable:
			printf("Type is not substitutable\n");
			break;
		case slkc::CompilationErrorKind::RequiresCompTimeExpr:
			printf("Requires a compile-time expression\n");
			break;
		case slkc::CompilationErrorKind::TypeArgTypeMismatched:
			printf("Type of type arguments mismatched\n");
			break;
		case slkc::CompilationErrorKind::InterfaceMethodsConflicted:
			printf("Interface methods conflicted\n");
			break;
		case slkc::CompilationErrorKind::TypeIsNotInitializable:
			printf("The type is not initializable\n");
			break;
		case slkc::CompilationErrorKind::MemberIsNotAccessible:
			printf("The member is not accessible\n");
			break;
		case slkc::CompilationErrorKind::InvalidEnumBaseType:
			printf("Invalid enumeration base type\n");
			break;
		case slkc::CompilationErrorKind::EnumItemIsNotAssignable:
			printf("Enumeration item is not assignable\n");
			break;
		case slkc::CompilationErrorKind::IncompatibleInitialValueType:
			printf("Incompatible initial value type\n");
			break;
		case slkc::CompilationErrorKind::ImportLimitExceeded:
			printf("Import item number exceeded\n");
			break;
		case slkc::CompilationErrorKind::ErrorParsingImportedModule: {
			const slkc::ErrorParsingImportedModuleErrorExData &exData = std::get<slkc::ErrorParsingImportedModuleErrorExData>(error.exData);

			printf("Error parsing imported module:\n");
			if (exData.lexicalError) {
				dumpLexicalError(*exData.lexicalError, indentLevel + 1);
			} else {
				for (auto &i : exData.mod->parser->syntaxErrors) {
					dumpSyntaxError(exData.mod->parser.get(), i, indentLevel + 1);
				}
			}
			break;
		}
		case slkc::CompilationErrorKind::ModuleNotFound:
			printf("Module not found\n");
			break;
		default:
			printf("Unknown error (%d)\n", (int)error.errorKind);
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
			slake::HostObjectRef<slake::ModuleObject> modObj = slake::ModuleObject::alloc(runtime.get());
			modObj->setAccess(slake::ACCESS_PUBLIC | slake::ACCESS_STATIC);

			peff::SharedPtr<slkc::Parser> parser;
			if (isBCMode) {
				if (!(parser = peff::makeShared<slkc::bc::BCParser>(peff::getDefaultAlloc(), document, tokenList.release(), peff::getDefaultAlloc()).castTo<slkc::Parser>())) {
					printError("Error allocating memory for the parser");
					return ENOMEM;
				}
			} else {
				if (!(parser = peff::makeShared<slkc::Parser>(peff::getDefaultAlloc(), document, tokenList.release(), peff::getDefaultAlloc()))) {
					printError("Error allocating memory for the parser");
					return ENOMEM;
				}
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
				dumpSyntaxError(parser.get(), *e);
			}

			for (auto &i : parser->syntaxErrors) {
				encounteredErrors = true;
				dumpSyntaxError(parser.get(), i);
			}

			if (auto e = completeParentModules(&compileEnv, moduleName.get(), mod); e) {
				encounteredErrors = true;
				dumpCompilationError(parser, *e);
			}

			if (auto e = indexModuleVarMembers(&compileEnv, rootMod); e) {
				encounteredErrors = true;
				dumpCompilationError(parser, *e);
			}

			if (auto e = slkc::compileModuleLikeNode(&compileEnv, mod, modObj.get()); e) {
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
			slkc::Decompiler decompiler;

			decompiler.dumpCfg = true;

			if (!decompiler.decompileModule(peff::getDefaultAlloc(), &dumpWriter, modObj.get())) {
				puts("Error dumping compiled module!");
			}

			document->rootModule = {};
			document->genericCacheDir.clear();
			document->externalModuleProviders.clear();
		}
	}

	return 0;
}
