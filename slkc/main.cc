#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <slkparse.hh>

class ArgumentError : public std::runtime_error {
public:
	inline ArgumentError(std::string msg) : runtime_error(msg){};
	virtual inline ~ArgumentError() {}
};

extern std::FILE* yyin;
Slake::Compiler::parser::location_type yylloc;
std::shared_ptr<Slake::Compiler::parser> yyparser;
int yylex_destroy(void);

static inline void msgPrintf(const char* msg, ...) noexcept {
	std::va_list args;
	va_start(args, msg);
	std::printf("slkc: ");
	std::vprintf(msg, args);
	va_end(args);
}

static inline void msgPuts(const char* msg) noexcept {
	std::printf("slkc: %s\n", msg);
}

void Slake::Compiler::parser::error(const Slake::Compiler::parser::location_type& loc, const std::string& msg) {
	std::fprintf(stderr, "Error at %d,%d: %s\n", loc.begin.line, loc.begin.column, msg.c_str());
}

static inline char* fetchArg(int argc, char** argv, int& i) {
	if (i >= argc)
		throw ArgumentError("Missing arguments");
	return argv[i++];
}

int main(int argc, char** argv) {
	Slake::Debug::setupMemoryLeakDetector();

	try {
		std::string srcPath = "", outPath = "a.out";

		try {
			for (int i = 1; i < argc; i++) {
				std::string arg = fetchArg(argc, argv, i);

				if (arg == "--include" || arg == "-I") {
					std::string path = fetchArg(argc, argv, i);
				} else
					srcPath = arg;
			}
		} catch (ArgumentError& e) {
			msgPuts(e.what());
			return EINVAL;
		}

		if (!srcPath.length()) {
			msgPuts("Missing input file");
			return EINVAL;
		}

		if (!(yyin = std::fopen(srcPath.c_str(), "rb"))) {
			msgPrintf("Error opening file `%s'", srcPath.c_str());
			return ENOENT;
		}

		auto rootScope = std::make_shared<Slake::Compiler::Scope>();
		Slake::Compiler::currentScope = rootScope;
		yyparser = std::make_shared<Slake::Compiler::parser>();

		auto cleanup = []() {
			Slake::Compiler::deinit();
			yylex_destroy();
			yyparser.reset();
		};

		auto parseResult = yyparser->parse();

		fclose(yyin);

		if (parseResult) {
			cleanup();
			return -1;
		}

		printf("%s\n", std::to_string(*Slake::Compiler::currentScope).c_str());

		cleanup();
	} catch (std::bad_alloc) {
		msgPuts("Out of memory");
		return ENOMEM;
	}

	Slake::Debug::dumpMemoryLeaks();

	return 0;
}
