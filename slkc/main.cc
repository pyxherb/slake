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
	void *user_data;
};

struct SingleArgOption;

typedef int (*ArglessOptionCallback)(const OptionMatchContext &match_context, const char *option);
typedef int (*SingleArgOptionCallback)(const OptionMatchContext &match_context, const char *option, const char *arg);
typedef int (*CustomOptionCallback)(OptionMatchContext &match_context, const char *option);
typedef int (*FallbackOptionCallback)(OptionMatchContext &match_context, const char *option);
typedef void (*RequireOptionArgCallback)(const OptionMatchContext &match_context, const SingleArgOption &option);

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
	peff::HashMap<std::string_view, const ArglessOption *> argless_options;
	peff::HashMap<std::string_view, const SingleArgOption *> single_arg_options;
	peff::HashMap<std::string_view, const CustomOption *> custom_options;
	FallbackOptionCallback fallback_option_callback;
	RequireOptionArgCallback require_option_arg_callback;

	SLAKE_FORCEINLINE CompiledOptionMap(peff::Alloc *alloc, FallbackOptionCallback fallback_option_callback, RequireOptionArgCallback require_option_arg_callback) noexcept : argless_options(alloc), single_arg_options(alloc), custom_options(alloc), fallback_option_callback(fallback_option_callback), require_option_arg_callback(require_option_arg_callback) {}
};

[[nodiscard]] bool build_option_map(
	CompiledOptionMap &option_map_out,
	const ArglessOptionMap &argless_options,
	const SingleArgOptionMap &single_arg_options,
	const CustomOptionMap &custom_options) {
	for (const auto &i : argless_options) {
		if (!option_map_out.argless_options.insert(std::string_view(i.name), &i)) {
			return false;
		}
	}

	for (const auto &i : single_arg_options) {
		if (!option_map_out.single_arg_options.insert(std::string_view(i.name), &i)) {
			return false;
		}
	}

	for (const auto &i : custom_options) {
		if (!option_map_out.custom_options.insert(std::string_view(i.name), &i)) {
			return false;
		}
	}

	return true;
}

[[nodiscard]] int match_args(const CompiledOptionMap &option_map, int argc, char **argv, void *user_data) {
	OptionMatchContext match_context = { argc, argv, 0, user_data };
	for (int i = 1; i < argc; ++i) {
		if (auto it = option_map.argless_options.find(std::string_view(argv[i])); it != option_map.argless_options.end()) {
			if (int result = it.value()->callback(match_context, argv[i]); result) {
				return result;
			}

			continue;
		}

		if (auto it = option_map.single_arg_options.find(std::string_view(argv[i])); it != option_map.single_arg_options.end()) {
			const char *opt = argv[i];
			if (++i == argc) {
				option_map.require_option_arg_callback(match_context, *it.value());
				return EINVAL;
			}

			if (int result = it.value()->callback(match_context, opt, argv[i]); result) {
				return result;
			}

			continue;
		}

		if (auto it = option_map.custom_options.find(std::string_view(argv[i])); it != option_map.custom_options.end()) {
			if (int result = it.value()->callback(match_context, argv[i]); result) {
				return result;
			}

			continue;
		}

		if (int result = option_map.fallback_option_callback(match_context, argv[i]); result) {
			return result;
		}
	}

	return 0;
}

#define print_error(fmt, ...) fprintf(stderr, "Error: " fmt, ##__VA_ARGS__)

struct MatchUserData {
	peff::DynArray<peff::String> *include_dirs;
};

bool is_bcmode = false;

const ArglessOptionMap g_argless_options = {
	{ "-bc", [](const OptionMatchContext &match_context, const char *option) -> int {
		 is_bcmode = true;
		 return 0;
	 } }
};

const char *g_mod_file_name = nullptr, *g_output_file_name = nullptr;

const SingleArgOptionMap g_single_arg_options = {
	{ "-I", [](const OptionMatchContext &match_context, const char *option, const char *arg) -> int {
		 MatchUserData *user_data = ((MatchUserData *)match_context.user_data);

		 peff::String dir(peff::default_allocator());

		 if (!dir.build(arg)) {
			 print_error("Out of memory");
			 return ENOMEM;
		 }

		 if (!user_data->include_dirs->push_back(std::move(dir))) {
			 print_error("Out of memory");
			 return ENOMEM;
		 }

		 return 0;
	 } },
	{ "-CTS", [](const OptionMatchContext &match_context, const char *option, const char *arg) -> int {
		 MatchUserData *user_data = ((MatchUserData *)match_context.user_data);

		 std::string_view s(arg);

		 size_t size = 0;
		 bool encountered_unit = false;

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
					 if (encountered_unit) {
						 print_error("Invalid stack size");
						 return EINVAL;
					 }
					 if (size >= SIZE_MAX / 10) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size += s[i] - '0';
					 break;
				 case 'K':
					 if (size >= SIZE_MAX / 1024) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024;
					 encountered_unit = true;
					 break;
				 case 'M':
					 if (size >= SIZE_MAX / 1024 / 1024) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024;
					 encountered_unit = true;
					 break;
				 case 'G':
					 if (size >= SIZE_MAX / 1024 / 1024 / 1024) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024 * 1024;
					 encountered_unit = true;
					 break;
				 default:
					 print_error("Invalid stack size");
					 return EINVAL;
			 }
		 }

		 if (!size) {
			 print_error("Invalid stack size");
			 return EINVAL;
		 }

		 slkc::sz_default_compile_thread_stack = size;

		 return 0;
	 } },
	{ "-PTS", [](const OptionMatchContext &match_context, const char *option, const char *arg) -> int {
		 MatchUserData *user_data = ((MatchUserData *)match_context.user_data);

		 std::string_view s(arg);

		 size_t size = 0;
		 bool encountered_unit = false;

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
					 if (encountered_unit) {
						 print_error("Invalid stack size");
						 return EINVAL;
					 }
					 if (size >= SIZE_MAX / 10) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size += s[i] - '0';
					 break;
				 case 'K':
					 if (size >= SIZE_MAX / 1024) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024;
					 encountered_unit = true;
					 break;
				 case 'M':
					 if (size >= SIZE_MAX / 1024 / 1024) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024;
					 encountered_unit = true;
					 break;
				 case 'G':
					 if (size >= SIZE_MAX / 1024 / 1024 / 1024) {
						 print_error("Stack size exceeds hardware memory limit");
						 return EINVAL;
					 }
					 size *= 1024 * 1024 * 1024;
					 encountered_unit = true;
					 break;
				 default:
					 print_error("Invalid stack size");
					 return EINVAL;
			 }
		 }

		 if (!size) {
			 print_error("Invalid stack size");
			 return EINVAL;
		 }

		 slkc::sz_default_parse_thread_stack = size;

		 return 0;
	 } },
	{ "-o", [](const OptionMatchContext &match_context, const char *option, const char *arg) -> int {
		 g_output_file_name = arg;

		 return 0;
	 } }
};

const CustomOptionMap g_custom_options = {

};

void dump_lexical_error(const slkc::LexicalError &lexical_error, int indent_level = 0) {
	for (int i = 0; i < indent_level; ++i) {
		putc('\t', stderr);
	}

	print_error("Syntax error at %zu, %zu: ",
		lexical_error.location.begin_position.line + 1,
		lexical_error.location.begin_position.column + 1);
	switch (lexical_error.kind) {
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

void dump_syntax_error(slkc::Parser *parser, const slkc::SyntaxError &syntax_error, int indent_level = 0) {
	const slkc::Token *begin_token = parser->token_list.at(syntax_error.token_range.begin_index).get();
	const slkc::Token *end_token = parser->token_list.at(syntax_error.token_range.end_index).get();

	for (int i = 0; i < indent_level; ++i) {
		putc('\t', stderr);
	}

	size_t line = begin_token->source_location.begin_position.line + 1;
	size_t column = begin_token->source_location.begin_position.column + 1;

	print_error("Syntax error at %zu, %zu: ", line, column);

	switch (syntax_error.error_kind) {
		case slkc::SyntaxErrorKind::OutOfMemory:
			printf("Out of memory\n");
			break;
		case slkc::SyntaxErrorKind::UnexpectedToken:
			printf("Unexpected token\n");
			break;
		case slkc::SyntaxErrorKind::ExpectingSingleToken:
			printf("Expecting %s\n",
				slkc::get_token_name(std::get<slkc::ExpectingSingleTokenErrorExData>(syntax_error.ex_data).expecting_token_id));
			break;
		case slkc::SyntaxErrorKind::ExpectingTokens: {
			printf("Expecting ");

			const slkc::ExpectingTokensErrorExData &ex_data = std::get<slkc::ExpectingTokensErrorExData>(syntax_error.ex_data);

			if (ex_data.expecting_token_ids.size()) {
				auto it = ex_data.expecting_token_ids.begin();

				fprintf(stderr, "%s", slkc::get_token_name(*it));

				while (++it != ex_data.expecting_token_ids.end()) {
					fprintf(stderr, " or %s", slkc::get_token_name(*it));
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

			const slkc::ConflictingDefinitionsErrorExData &ex_data = std::get<slkc::ConflictingDefinitionsErrorExData>(syntax_error.ex_data);

			fprintf(stderr, "%s' conflicts with other definitions\n", ex_data.member_name.data());
			break;
		}
		default:
			printf("Unknown error (%d)\n", (int)syntax_error.error_kind);
			break;
	}
}

void dump_compilation_error(peff::SharedPtr<slkc::Parser> parser, const slkc::CompilationError &error, int indent_level = 0) {
	const slkc::Token *begin_token = parser->token_list.at(error.token_range.begin_index).get();
	const slkc::Token *end_token = parser->token_list.at(error.token_range.end_index).get();

	for (int i = 0; i < indent_level; ++i) {
		putc('\t', stderr);
	}

	print_error("Error at %zu, %zu to %zu, %zu: ",
		begin_token->source_location.begin_position.line + 1, begin_token->source_location.begin_position.column + 1,
		end_token->source_location.end_position.line + 1, end_token->source_location.end_position.column + 1);
	switch (error.error_kind) {
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
		case slkc::CompilationErrorKind::ExpectingTypeName:
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
		case slkc::CompilationErrorKind::FunctionOverloadingDuplicatedDuringInstantiation:
			printf("Duplicated function overloading detected during instantiation\n");
			break;
		case slkc::CompilationErrorKind::ReturnValueTypeDoesNotMatch:
			printf("Return value type does not match\n");
			break;

		case slkc::CompilationErrorKind::ImportLimitExceeded:
			printf("Import item number exceeded\n");
			break;
		case slkc::CompilationErrorKind::ErrorParsingImportedModule: {
			const slkc::ErrorParsingImportedModuleErrorExData &ex_data = std::get<slkc::ErrorParsingImportedModuleErrorExData>(error.ex_data);

			printf("Error parsing imported module:\n");
			if (ex_data.lexical_error) {
				dump_lexical_error(*ex_data.lexical_error, indent_level + 1);
			} else {
				for (auto &i : ex_data.mod->parser->syntax_errors) {
					dump_syntax_error(ex_data.mod->parser.get(), i, indent_level + 1);
				}
			}
			break;
		}
		case slkc::CompilationErrorKind::ModuleNotFound:
			printf("Module not found\n");
			break;
		default:
			printf("Unknown error (%d)\n", (int)error.error_kind);
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

	peff::DynArray<peff::String> include_dirs(peff::default_allocator());
	{
		CompiledOptionMap option_map(
			peff::default_allocator(),
			[](OptionMatchContext &match_context, const char *option) -> int {
				if (g_mod_file_name) {
					print_error("Duplicated target file name");
					return EINVAL;
				}

				g_mod_file_name = option;

				return 0;
			},
			[](const OptionMatchContext &match_context, const SingleArgOption &option) {
				print_error("Option `%s' requires more arguments", option.name);
			});

		if (!build_option_map(option_map, g_argless_options, g_single_arg_options, g_custom_options)) {
			print_error("Out of memory");
			return ENOMEM;
		}

		{
			MatchUserData match_user_data = {};
			match_user_data.include_dirs = &include_dirs;

			if (int result = match_args(option_map, argc, argv, &match_user_data); result) {
				return result;
			}
		}
	}

	if (!g_mod_file_name) {
		print_error("Missing target file name");
		return EINVAL;
	}

	if (!g_output_file_name) {
		print_error("Missing output file name");
		return EINVAL;
	}

	FILE *fp = fopen(g_mod_file_name, "rb");

	if (!fp) {
		print_error("Error opening the file");
		return EIO;
	}

	peff::ScopeGuard close_fp_guard([fp]() noexcept {
		fclose(fp);
	});

	if (fseek(fp, 0, SEEK_END)) {
		print_error("Error evaluating file size");
		return EIO;
	}

	long file_size;
	if ((file_size = ftell(fp)) < 1) {
		print_error("Error evaluating file size");
		return EIO;
	}

	if (fseek(fp, 0, SEEK_SET)) {
		print_error("Error evaluating file size");
		return EIO;
	}

	{
		auto deleter = [](char *ptr) {
			free(ptr);
		};
		std::unique_ptr<char[], decltype(deleter)> buf((char *)malloc((size_t)file_size + 1), deleter);

		if (!buf) {
			print_error("Error allocating memory for reading the file");
			return ENOMEM;
		}

		(buf.get())[file_size] = '\0';

		if (fread(buf.get(), file_size, 1, fp) < 1) {
			print_error("Error reading the file");
			return EIO;
		}

		peff::SharedPtr<slkc::Document> document(peff::make_shared<slkc::Document>(peff::default_allocator(), peff::default_allocator()));

		peff::SharedPtr<slkc::FileSystemExternalModuleProvider> fs_external_mod_provider;

		if (!(fs_external_mod_provider = peff::make_shared<slkc::FileSystemExternalModuleProvider>(peff::default_allocator(), peff::default_allocator()))) {
			print_error("Out of memory");
			return ENOMEM;
		}

		for (auto &i : include_dirs) {
			if (!fs_external_mod_provider->import_paths.push_back(std::move(i))) {
				print_error("Out of memory");
				return ENOMEM;
			}
		}

		include_dirs.clear_and_shrink();

		if (!document->external_module_providers.push_back(fs_external_mod_provider.cast_to<slkc::ExternalModuleProvider>())) {
			print_error("Out of memory");
			return ENOMEM;
		}

		slkc::AstNodePtr<slkc::ModuleNode> mod;
		if (!(mod = slkc::make_ast_node<slkc::ModuleNode>(peff::default_allocator(), peff::default_allocator(), document))) {
			print_error("Error allocating memory for the target module");
			return ENOMEM;
		}
		if (!mod->alloc_scope()) {
			print_error("Error allocating memory for the target module");
			return ENOMEM;
		}

		document->main_module = mod.get();

		slkc::TokenList token_list(peff::default_allocator());
		{
			slkc::Lexer lexer(peff::default_allocator());

			std::string_view sv(buf.get(), file_size);

			if (auto e = lexer.lex(mod.get(), sv, peff::default_allocator(), document); e) {
				dump_lexical_error(*e);
				return -1;
			}

			token_list = std::move(lexer.token_list);
		}

		std::unique_ptr<slake::Runtime, peff::DeallocableDeleter<slake::Runtime>> runtime(
			slake::Runtime::alloc(peff::default_allocator(), peff::default_allocator()));
		if (!runtime) {
			print_error("Error allocating memory for the runtime");
			return ENOMEM;
		}
		{
			{
				slake::HostObjectRef<slake::ModuleObject> mod_obj = slake::ModuleObject::alloc(runtime.get());
				mod_obj->set_access(slake::ACCESS_PUBLIC | slake::ACCESS_STATIC);

				peff::SharedPtr<slkc::Parser> parser;
				if (!(parser = peff::make_shared<slkc::Parser>(peff::default_allocator(), document, std::move(token_list), peff::default_allocator()))) {
					print_error("Error allocating memory for the parser");
					return ENOMEM;
				}

				slkc::AstNodePtr<slkc::ModuleNode> root_mod;
				if (!(root_mod = slkc::make_ast_node<slkc::ModuleNode>(peff::default_allocator(), peff::default_allocator(), document))) {
					print_error("Error allocating memory for the root module");
					return ENOMEM;
				}
				if (!root_mod->alloc_scope()) {
					print_error("Error allocating memory for the root module");
					return ENOMEM;
				}
				document->root_module = root_mod;

				slkc::IdRefPtr module_name;

				bool encountered_errors = false;
				if (auto e = parser->parse(mod, module_name); e) {
					encountered_errors = true;
					dump_syntax_error(parser.get(), *e);
				}

				for (auto &i : parser->syntax_errors) {
					encountered_errors = true;
					dump_syntax_error(parser.get(), i);
				}

				slkc::CompileEnv compile_env(runtime.get(), document, &peff::g_null_alloc, peff::default_allocator());
				if (module_name) {
					if (auto e = complete_parent_modules(&compile_env, module_name.get(), mod); e) {
						encountered_errors = true;
						dump_compilation_error(parser, *e);
					}
				}

				if (auto e = slkc::compile_module_like_node(&compile_env, mod, mod_obj.get()); e) {
					encountered_errors = true;
					dump_compilation_error(parser, *e);
				}

				// Sort errors in order.
				std::sort(compile_env.errors.data(), compile_env.errors.data() + compile_env.errors.size());

				for (auto &i : compile_env.errors) {
					encountered_errors = true;
					dump_compilation_error(parser, i);
				}

				if (!encountered_errors) {
					slake::HostObjectRef<slake::ModuleObject> last_module = runtime->get_root_object();

					for (size_t i = 0; i < module_name->entries.size() - 1; ++i) {
						slkc::IdRefEntry &e = module_name->entries.at(i);

						if (auto cur_mod = last_module->get_member(e.name); cur_mod) {
							last_module = (slake::ModuleObject *)cur_mod.as_object;

							continue;
						}

						slake::HostObjectRef<slake::ModuleObject> cur_module;

						if (!(cur_module = slake::ModuleObject::alloc(runtime.get()))) {
							puts("Error dumping compiled module!");
						}

						if (!cur_module->set_name(e.name)) {
							puts("Error dumping compiled module!");
						}

						if (last_module) {
							if (!last_module->add_member(cur_module.get())) {
								puts("Error dumping compiled module!");
							}
							cur_module->set_parent(last_module.get());
						}

						last_module = cur_module;
					}

					if (!mod_obj->set_name(module_name->entries.back().name)) {
						puts("Error dumping compiled module!");
					}

					if (!last_module->add_member(mod_obj.get())) {
						puts("Error dumping compiled module!");
					}
					mod_obj->set_parent(last_module.get());

					FILE *fp;

					if (!(fp = fopen(g_output_file_name, "wb"))) {
						print_error("Error opening the output file");
					}
					FileWriter w(fp);
					if (auto e = slkc::dump_module(peff::default_allocator(), &w, mod_obj.get())) {
						encountered_errors = true;
						dump_compilation_error(parser, *e);
					}

					ANSIDumpWriter dump_writer;
					slkc::Decompiler decompiler;

					// decompiler.dump_cfg = true;

					if (!decompiler.decompile_module(peff::default_allocator(), &dump_writer, mod_obj.get())) {
						puts("Error dumping compiled module!");
					}
				}
			}

			// The document must be cleared manually at the end or the memory will leak!
			document->root_module.reset();
			document->generic_cache_dir.clear();
			document->external_module_providers.clear_and_shrink();
			mod.reset();
			fs_external_mod_provider.reset();
			// document->clear_deferred_destructible_ast_nodes();
			document.reset();
		}
	}

	return 0;
}
