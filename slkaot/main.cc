#include "compiler.h"
#include <slake/util/debug.h>
#include <deque>
#include <cstring>

#include <filesystem>
#include <fstream>

using namespace slake::slkaot;

class ArgumentError : public std::runtime_error {
public:
	inline ArgumentError(std::string msg) : runtime_error(msg){};
	virtual ~ArgumentError() = default;
};

static inline char *fetchArg(int argc, char **argv, int &i) {
	if (i >= argc)
		throw ArgumentError("Missing arguments");
	return argv[i++];
}

enum class AppAction : uint8_t {
	Compile = 0,
};

std::string srcPath = "", outPath = "";

AppAction action = AppAction::Compile;
std::deque<std::string> modulePaths;

struct CmdLineAction {
	const char *options;
	void (*fn)(int argc, char **argv, int &i);
};

CmdLineAction cmdLineActions[] = {
	{ "-I\0"
	  "--module-path\0",
		[](int argc, char **argv, int &i) {
			modulePaths.push_back(fetchArg(argc, argv, i));
		} }
};

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

	try {
		try {
			for (int i = 1; i < argc;) {
				std::string arg = fetchArg(argc, argv, i);

				for (uint16_t j = 0; j < sizeof(cmdLineActions) / sizeof(cmdLineActions[0]); j++) {
					for (auto k = cmdLineActions[j].options; *k; k += strlen(k) + 1)
						if (!strcmp(k, arg.c_str())) {
							cmdLineActions[j].fn(argc, argv, i);
							goto succeed;
						}
				}

				srcPath = arg;
			succeed:;
			}
		} catch (ArgumentError &e) {
			fprintf(stderr, "Error: %s\n", e.what());
			return EINVAL;
		}

		switch (action) {
			case AppAction::Compile: {
				if (!srcPath.length()) {
					fputs("Error: Missing input file\n", stderr);
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

				std::ifstream is;
				std::ofstream os;

				is.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
				os.exceptions(std::ios::failbit | std::ios::badbit);

				is.open(srcPath, std::ios::binary);
				os.open(outPath, std::ios::binary);

				is.close();
				os.close();

				break;
			}
			default:
				std::terminate();
		}
	} catch (std::bad_alloc) {
		perror("Out of memory");
		return ENOMEM;
	}

	return 0;
}
