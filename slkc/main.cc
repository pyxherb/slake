#include "compiler/compiler.h"
#include "decompiler/decompiler.h"
#include "lsp/lsp.h"

#include <filesystem>
#include <fstream>

#include <slake/util/debug.h>

using namespace slake::slkc;

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

enum AppAction : uint8_t {
	ACT_COMPILE = 0,
	ACT_DUMP,
	ACT_LSP
};

std::string srcPath = "", outPath = "";

AppAction action = ACT_COMPILE;
std::deque<std::string> modulePaths;
uint16_t lspServerPort = 8080;

struct CmdLineAction {
	const char *options;
	void (*fn)(int argc, char **argv, int &i);
};

CmdLineAction cmdLineActions[] = {
	{ "-I\0"
	  "--module-path\0",
		[](int argc, char **argv, int &i) {
			std::string path = fetchArg(argc, argv, i);
		} },
	{ "-d\0"
	  "--dump\0",
		[](int argc, char **argv, int &i) {
			action = ACT_DUMP;
		} },
	{ "-l\0"
	  "--no-source-location-info\0",
		[](int argc, char **argv, int &i) {
			slake::decompiler::decompilerFlags |= slake::decompiler::DECOMP_SRCLOCINFO;
		} },
	{ "-s\0"
	  "--language-server\0",
		[](int argc, char **argv, int &i) {
			action = ACT_LSP;
		} },
	{ "-p\0"
	  "--server-port",
		[](int argc, char** argv, int& i) {
			uint32_t port = strtoul(fetchArg(argc, argv, i), nullptr, 10);

			if (port > UINT16_MAX)
				throw ArgumentError("Invalid port number");

			lspServerPort = port;
		} }
};

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

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
			fprintf(stderr, "Error: %s\n", e.what());
			return EINVAL;
		}

		switch (action) {
			case ACT_COMPILE: {
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

				std::ifstream is(srcPath, std::ios::binary);
				std::ofstream os(outPath, std::ios::binary | std::ios::out);

				std::unique_ptr<Compiler> compiler = std::make_unique<Compiler>();

				try {
					compiler->compile(is, os);
				} catch (FatalCompilationError e) {
					fprintf(stderr, "Error at %zd, %zd: %s\n", e.message.loc.line, e.message.loc.column, e.message.msg.c_str());
					return -1;
				}

				is.close();
				os.close();
				break;
			}
			case ACT_DUMP: {
				std::ifstream fs(srcPath, std::ios::binary);

				slake::decompiler::decompile(fs, std::cout);

				fs.close();
				break;
			}
			case ACT_LSP:
				return slake::slkc::lsp::lspServerMain(lspServerPort);
			default:
				assert(false);
		}
	} catch (std::bad_alloc) {
		perror("Out of memory");
		return ENOMEM;
	}

	return 0;
}
