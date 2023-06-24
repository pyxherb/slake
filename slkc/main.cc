#include "compiler/compiler.h"
#include "decompiler/decompiler.h"

#include <filesystem>
#include <fstream>

#include <slake/util/debug.h>

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

enum AppAction : uint8_t {
	ACT_COMPILE = 0,
	ACT_DUMP
};

std::string srcPath = "", outPath = "";

AppAction action = ACT_COMPILE;
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
		} },
	{ "-d\0"
	  "--dump\0",
		[](int argc, char **argv, int &i) {
			action = ACT_DUMP;
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
			fprintf(stderr, "Error: %s", e.what());
			return EINVAL;
		}

		switch (action) {
			case ACT_COMPILE: {
				if (!srcPath.length()) {
					perror("Error: Missing input file");
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

				std::ifstream fs(srcPath, std::ios::binary);
				antlr4::ANTLRInputStream is(fs);

				SlakeLexer lexer(&is);
				antlr4::CommonTokenStream tokens(&lexer);

				SlakeParser parser(&tokens);
				auto tree = parser.prog();

				printf("%s\n", tree->toStringTree(&parser).c_str());

				Slake::Compiler::AstVisitor visitor;
				visitor.visit(tree);
				fs.close();
				break;
			}
			case ACT_DUMP: {
				std::ifstream fs(srcPath, std::ios::binary);

				Slake::Decompiler::decompile(fs, std::cout);

				fs.close();
				break;
			}
			default:
				assert(false);
		}
	} catch (std::bad_alloc) {
		perror("Out of memory");
		return ENOMEM;
	}

	return 0;
}
