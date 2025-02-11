#include "compiler.h"
#include <slake/util/debug.h>
#include <deque>
#include <cstring>

#include <filesystem>
#include <fstream>

using namespace slake;
using namespace slake::slkaot;

class ArgumentError : public std::runtime_error {
public:
	inline ArgumentError(std::string msg) : runtime_error(msg) {};
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

std::string srcPath = "", headerOutPath = "", sourceOutPath = "";

AppAction action = AppAction::Compile;

struct CmdLineAction {
	const char *options;
	void (*fn)(int argc, char **argv, int &i);
};

CmdLineAction cmdLineActions[] = {
	{ "-I\0"
	  "--module-path\0",
		[](int argc, char **argv, int &i) {
			modulePaths.push_back(fetchArg(argc, argv, i));
		} },
	{ "-H\0"
	  "--header-output\0",
		[](int argc, char **argv, int &i) {
			headerOutPath = fetchArg(argc, argv, i);
		} },
	{ "-S\0"
	  "--source-output\0",
		[](int argc, char **argv, int &i) {
			sourceOutPath = fetchArg(argc, argv, i);
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

				if (!headerOutPath.length()) {
					fputs("Error: Missing header output path\n", stderr);
					return EINVAL;
				}

				if (!sourceOutPath.length()) {
					fputs("Error: Missing source output path\n", stderr);
					return EINVAL;
				}

				std::ifstream is;
				std::ofstream ohs, oss;
				// std::ofstream os;

				is.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
				is.open(srcPath, std::ios::binary);

				ohs.exceptions(std::ios::failbit | std::ios::badbit);
				ohs.open(headerOutPath, std::ios::binary);

				oss.exceptions(std::ios::failbit | std::ios::badbit);
				oss.open(sourceOutPath, std::ios::binary);

				auto rt = std::make_unique<slake::Runtime>(peff::getDefaultAlloc());
				rt->setModuleLocator(moduleLocator);

				HostObjectRef<ModuleObject> mod;
				try {
					mod = rt->loadModule(is, LMOD_NOIMPORT);
				} catch (slake::LoaderError e) {
					std::cerr << "Error loading the module: " << e.what() << std::endl;
					return -1;
				}

				bc2cxx::BC2CXX bc2cxxCompiler;

				auto result = bc2cxxCompiler.compile(mod.get());

				bc2cxxCompiler.dumpAstNode(ohs, result.first, bc2cxx::BC2CXX::ASTDumpMode::Header);
				bc2cxxCompiler.dumpAstNode(oss, result.second, bc2cxx::BC2CXX::ASTDumpMode::Source);

				is.close();
				// os.close();

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
