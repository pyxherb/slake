#include <bcparse.hh>
#include <bclex.h>
#include <fstream>

using namespace Slake;
#define printmsg(msg, ...) std::printf("slkbcc: " msg, ##__VA_ARGS__)
#define msgputs(msg) std::printf("slkbcc: %s\n", msg)

void Assembler::parser::error(const Assembler::parser::location_type &loc, const std::string &msg) {
	std::fprintf(stderr, "Error at %d,%d: %s\n", loc.begin.line, loc.begin.column, msg.c_str());
}

class ArgumentError : public std::runtime_error {
public:
	inline ArgumentError(std::string msg) : runtime_error(msg){};
	virtual inline ~ArgumentError() {}
};

static inline char *fetchArg(int argc, char **argv, int &i) {
	if (i >= argc)
		throw ArgumentError("Missing arguments");
	return argv[i++];
}

std::string srcPath = "", outPath = "";
std::vector<std::string> modulePaths;

struct CmdLineAction {
	const char *options;
	void (*fn)(int argc, char **argv, int &i);
};

CmdLineAction cmdLineActions[] = {
	{ "-I\0"
	  "--include\0",
		[](int argc, char **argv, int &i) {
			std::string path = fetchArg(argc, argv, i);
		} }
};

int main(int argc, char **argv) {
	Slake::Util::setupMemoryLeakDetector();

	try {
		try {
			for (int i = 1; i < argc;) {
				std::string arg = fetchArg(argc, argv, i);

				for (uint16_t j = 0; j < sizeof(cmdLineActions) / sizeof(cmdLineActions[0]); j++) {
					for (auto k = cmdLineActions[j].options; *k; k += std::strlen(k))
						if (!std::strcmp(k, arg.c_str())) {
							cmdLineActions[j].fn(argc, argv, i);
							goto succeed;
						}
				}
			succeed:
				srcPath = arg;
			}
		} catch (ArgumentError &e) {
			msgputs(e.what());
			return EINVAL;
		}

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

		// The LEXER requires a C-styled file stream.
		if (!(yyin = std::fopen(srcPath.c_str(), "rb"))) {
			printmsg("Error opening file `%s'", srcPath.c_str());
			return ENOENT;
		}

		yyparser = std::make_shared<Assembler::parser>();

		rootScope = make_shared<Assembler::Scope>();
		curScope = rootScope;

		auto cleaner = []() {
			yylex_destroy();
			yyparser.reset();

			curScope.reset();
			rootScope.reset();
		};

		try {
			auto parseResult = yyparser->parse();

			if (parseResult) {
				cleaner();
				return -1;
			}

			// printf("%s\n", std::to_string(*Slake::Compiler::currentScope).c_str());

			std::fstream fs(outPath, std::ios::out | std::ios::binary);
			if (!fs.is_open()) {
				printmsg("Error opening file `%s'\n", srcPath.c_str());
				return ENOENT;
			}
			fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);

			Slake::Assembler::compile(fs);

			fs.close();
		} catch (Slake::Assembler::parser::syntax_error e) {
			yyparser->error(e.location, e.what());
		}
		fclose(yyin);
		cleaner();

	} catch (std::bad_alloc) {
		msgputs("Out of memory");
		return ENOMEM;
	}

	return 0;
}
