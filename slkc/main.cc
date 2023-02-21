#include <decompiler/state.hh>
#include <slkparse.hh>

class ArgumentError : public std::runtime_error {
public:
	inline ArgumentError(std::string msg) : runtime_error(msg){};
	virtual inline ~ArgumentError() {}
};

extern std::FILE* yyin;
int yylex_destroy(void);

#define printmsg(msg, ...) std::printf("slkc: " msg, ##__VA_ARGS__)
#define msgputs(msg) std::printf("slkc: %s\n", msg)

static void syntaxError(const Slake::Compiler::parser::location_type& loc, const std::string& msg) {
	std::fprintf(stderr, "Error at %d,%d: %s\n", loc.begin.line, loc.begin.column, msg.c_str());
}

void Slake::Compiler::parser::error(const Slake::Compiler::parser::location_type& loc, const std::string& msg) {
	syntaxError(loc, msg);
}

static inline char* fetchArg(int argc, char** argv, int& i) {
	if (i >= argc)
		throw ArgumentError("Missing arguments");
	return argv[i++];
}

enum AppAction : std::uint8_t {
	ACT_COMPILE = 0,
	ACT_DUMP
};

std::string srcPath = "", outPath = "";
AppAction action = ACT_COMPILE;

struct CmdLineAction {
	const char** options;
	void (*fn)(int argc, char** argv, int& i);
};

CmdLineAction cmdLineActions[] = {
	{ (const char*[]){
		  "-I",
		  "--include",
		  nullptr },
		[](int argc, char** argv, int& i) {
			std::string path = fetchArg(argc, argv, i);
		} },
	{ (const char*[]){
		  "-d",
		  "--dump",
		  nullptr },
		[](int argc, char** argv, int& i) {
			action = ACT_DUMP;
		} }
};

int main(int argc, char** argv) {
	Slake::Debug::setupMemoryLeakDetector();

	try {
		try {
			for (int i = 1; i < argc;) {
				std::string arg = fetchArg(argc, argv, i);

				for (std::uint16_t j = 0; j < sizeof(cmdLineActions) / sizeof(cmdLineActions[0]); j++) {
					for (auto k = cmdLineActions[j].options; *k; k++)
						if (!std::strcmp(*k, arg.c_str())) {
							cmdLineActions[j].fn(argc, argv, i);
							goto succeed;
						}
				}
			succeed:
				srcPath = arg;
			}
		} catch (ArgumentError& e) {
			msgputs(e.what());
			return EINVAL;
		}

		switch (action) {
			case ACT_COMPILE: {
				if (!srcPath.length()) {
					msgputs("Missing input file");
					return EINVAL;
				}
				if (!outPath.length()) {
					auto i = srcPath.find_last_of('.');
					if (i == srcPath.npos) {
						outPath = srcPath + ".slx";
					} else {
						outPath = srcPath.substr(0, i) + ".slx";
					}
				}

				// Because the LEXER requires a C-styled file stream,
				// We didn't use file I/O classes for C++.
				if (!(yyin = std::fopen(srcPath.c_str(), "rb"))) {
					printmsg("Error opening file `%s'", srcPath.c_str());
					return ENOENT;
				}

				auto rootScope = std::make_shared<Slake::Compiler::Scope>();
				Slake::Compiler::currentScope = rootScope;
				yyparser = std::make_shared<Slake::Compiler::parser>();

				auto cleaner = []() {
					Slake::Compiler::deinit();
					yylex_destroy();
					yyparser.reset();
				};

				try {
					auto parseResult = yyparser->parse();

					fclose(yyin);

					if (parseResult) {
						cleaner();
						return -1;
					}

					// printf("%s\n", std::to_string(*Slake::Compiler::currentScope).c_str());

					std::fstream fs(outPath, std::ios::out | std::ios::binary);
					if (!fs.is_open()) {
						printmsg("Error opening file `%s'\n", srcPath.c_str());
						break;
					}
					fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);

					Slake::Compiler::compile(rootScope, fs);
					fs.close();
				} catch (Slake::Compiler::parser::syntax_error e) {
					syntaxError(e.location, e.what());
				}
				cleaner();
				break;
			}
			case ACT_DUMP: {
				if (!srcPath.length()) {
					msgputs("Missing input file");
					return EINVAL;
				}

				std::fstream fs(srcPath, std::ios::in | std::ios::binary);
				if (!fs.is_open()) {
					printmsg("Error opening file `%s'\n", srcPath.c_str());
					break;
				}
				fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);

				try {
					Slake::Decompiler::decompile(fs);
				}
				catch (Slake::Decompiler::DecompileError e) {
					printf("%s\n", e.what());
				}

				fs.close();
				break;
			}
			default:
				assert(false);
		}


	} catch (std::bad_alloc) {
		msgputs("Out of memory");
		return ENOMEM;
	}

	return 0;
}
