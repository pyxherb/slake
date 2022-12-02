#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <spkparse.hh>
#include <stdexcept>

extern std::FILE* yyin;
extern int yylineno;
SpkC::Syntax::parser::location_type yylloc;
std::shared_ptr<SpkC::Syntax::parser> yyparser;
int yylex_destroy(void);

static void msgPrintf(const char* msg, ...) noexcept {
	std::va_list args;
	va_start(args, msg);
	std::printf("spkc: ");
	std::vprintf(msg, args);
	va_end(args);
}

static void msgPuts(const char* msg) noexcept {
	std::printf("spkc: ");
	std::puts(msg);
}

int main(int argc, char** argv) {
	Swampeak::Debug::setupMemoryLeakDetector();

	/*
	if (argc < 2) {
		msgPuts("Too few arguments");
		return EINVAL;
	}*/

	try {
		std::string srcPath = "D:/repos/thngine/assets/thngine/scripts/thngine/player.spk";
		std::string outPath = "a.out";
		for (int i = 2; i < argc; i++) {
		}

		FILE* fp;
		if (!(fp = std::fopen(srcPath.c_str(), "rb"))) {
			msgPrintf("Error opening file `%s'", srcPath.c_str());
			return ENOENT;
		}

		auto rootScope = std::make_shared<SpkC::Syntax::Scope>();
		SpkC::Syntax::currentScope = rootScope;

		yyin = fp;
		yyparser = std::make_shared<SpkC::Syntax::parser>();
		auto parseResult = yyparser->parse();

		fclose(fp);

		if (parseResult) {
			rootScope.reset();
			currentScope.reset();
			currentEnum.reset();
			currentClass.reset();
			yylex_destroy();

			yyparser.reset();
			return -1;
		}

		for (auto& i : SpkC::Syntax::currentScope->imports)
			printf("%s = %c\"%s\"", i.first.c_str(), i.second->searchInSystemPath ? '@' : '\0', i.second->path.c_str());

		rootScope.reset();
		currentScope.reset();
		currentEnum.reset();
		currentClass.reset();
		yylex_destroy();
		yyparser.reset();
	} catch (std::bad_alloc) {
		msgPuts("Out of memory");
		return ENOMEM;
	}

	Swampeak::Debug::dumpMemoryLeaks();

	return 0;
}

void SpkC::Syntax::parser::error(const SpkC::Syntax::parser::location_type& loc, const std::string& msg) {
	std::fprintf(stderr, "Error at %d,%d: %s\n", loc.begin.line, loc.begin.column, msg.c_str());
}
