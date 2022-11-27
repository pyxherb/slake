#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

#include <spkparse.hh>

extern std::FILE* yyin;
extern int yylineno;
extern YYLTYPE yylloc;
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
		if (!(fp = fopen(srcPath.c_str(), "rb"))) {
			msgPrintf("Error opening file `%s'", srcPath.c_str());
			return ENOENT;
		}

		auto rootScope = std::make_shared<SpkC::Syntax::Scope>();
		SpkC::Syntax::currentScope = rootScope;

		yyin = fp;
		auto parseResult = yyparse();

		yylex_destroy();
		fclose(fp);

		if (parseResult) {
			return -1;
		}

		for (auto &i : SpkC::Syntax::currentScope->imports)
			printf("%s = %c\"%s\"", i.first.c_str(), i.second->searchInSystemPath ? '@' : '\0', i.second->path.c_str());
	} catch (std::bad_alloc) {
		msgPuts("Out of memory");
		return ENOMEM;
	}

	Swampeak::Debug::dumpMemoryLeaks();

	return 0;
}

void yyerror(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	std::fprintf(stderr, "Error at %d, %d: ", yylloc.last_line, yylloc.last_column);
	std::vfprintf(stderr, fmt, args);
	putc('\n', stderr);

	va_end(args);
}
