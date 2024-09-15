#include <slake/util/debug.h>

#include "compiler/compiler.h"
#include "decompiler/decompiler.h"

#if SLKC_WITH_LANGUAGE_SERVER
	#include "server/server.h"
#endif

#include <filesystem>
#include <fstream>

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

enum class AppAction : uint8_t {
	Compile = 0,
	Dump,
	LanguageServer
};

std::string srcPath = "", outPath = "";

AppAction action = AppAction::Compile;
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
			modulePaths.push_back(fetchArg(argc, argv, i));
		} },
	{ "-d\0"
	  "--dump\0",
		[](int argc, char **argv, int &i) {
			action = AppAction::Dump;
		} },
	{ "-l\0"
	  "--no-source-location-info\0",
		[](int argc, char **argv, int &i) {
			slake::decompiler::decompilerFlags |= slake::decompiler::DECOMP_SRCLOC;
		} },
	{ "-s\0"
	  "--language-server\0",
		[](int argc, char **argv, int &i) {
			action = AppAction::LanguageServer;
		} },
	{ "-p\0"
	  "--server-port\0",
		[](int argc, char **argv, int &i) {
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

				std::unique_ptr<Compiler> compiler = std::make_unique<Compiler>();
				compiler->modulePaths = modulePaths;

				try {
					compiler->compile(is, os);
				} catch (FatalCompilationError e) {
					std::cerr << "Error at " << std::to_string(e.message.sourceLocation) << ": " << e.message.msg << std::endl;
					return -1;
				}

				bool foundErrors = false;
				for (auto &i : compiler->messages) {
					const char *msgType = "<Unknown Message Type>";
					switch (i.type) {
						case MessageType::Info:
							msgType = "Info";
							break;
						case MessageType::Note:
							msgType = "Note";
							break;
						case MessageType::Warn:
							msgType = "Warn";
							break;
						case MessageType::Error:
							foundErrors = true;
							msgType = "error";
							break;
					}

					std::cout << msgType << " at " << std::to_string(i.sourceLocation) << ": " << i.msg << std::endl;
				}

				is.close();
				os.close();

				if (foundErrors)
					return -1;
				break;
			}
			case AppAction::Dump: {
				std::ifstream fs(srcPath, std::ios::binary);

				slake::decompiler::decompile(fs, std::cout);

				fs.close();
				break;
			}
			case AppAction::LanguageServer:
#if SLKC_WITH_LANGUAGE_SERVER
			{
				printf("Language server started on local port %hu\n", lspServerPort);

				slake::slkc::Server server;
				server.modulePaths = modulePaths;

				server.start(lspServerPort);

				break;
			}
#else
				fputs("This version of SLKC is not compiled with language server support", stderr);
				break;
#endif
			default:
				assert(false);
		}
	} catch (std::bad_alloc) {
		perror("Out of memory");
		return ENOMEM;
	}

	return 0;
}
