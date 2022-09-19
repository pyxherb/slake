#include <vizard/debug.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

#include "syntax/lex.h"
#include "syntax/parse.h"

static void msgPrintf(const char* msg, ...) noexcept {
	std::va_list args;
	va_start(args, msg);
	std::printf("vzc: ");
	std::vprintf(msg, args);
	va_end(args);
}

static void msgPuts(const char* msg) noexcept {
	std::printf("vzc: ");
	std::puts(msg);
}

int main(int argc, char** argv) {
	Vzc::Debug::setupMemoryLeakDetector();

	if (argc < 2) {
		msgPuts("Too few arguments");
		return EINVAL;
	}

	try {
		std::string srcPath = argv[1];
		std::string outPath = "a.out";
		for (int i = 2; i < argc; i++) {
		}

		std::fstream fs;
		long long fileSize;
		char* fileContent;

		// Try to open the file
		fs.open(srcPath, std::ios::in | std::ios::binary);
		if (!fs.is_open()) {
			msgPrintf("Error opening file `%s'", srcPath.c_str());
			return ENOENT;
		}

		// Get file size
		{
			fs.seekg(0, std::ios::end);
			fileSize = fs.tellg();
			if (fileSize < 0) {
				fs.close();
				msgPrintf("Error seeking file `%s'", srcPath.c_str());
				return EIO;
			}
			fs.seekg(0, std::ios::beg);
		}

		// Read content of the file
		fileContent = new char[fileSize + 1];
		if (!fs.read(fileContent, fileSize)) {
			fs.close();
			delete[] fileContent;
			msgPrintf("Error reading file `%s'", srcPath.c_str());
			return EIO;
		}
		fileContent[fileSize] = '\0';
		fs.close();

		auto lexer = std::make_shared<Vzc::Syntax::Lexer>();
		auto parser = std::make_shared<Vzc::Syntax::VzcParser>();

		try {
			auto tokens = lexer->lex(fileContent);
			parser->parse(tokens);
		} catch (Blare::LexicalError e) {
			msgPrintf("Error at %zu, %zu: %s\n", e.line, e.column,
				e.what());
			return -1;
		} catch (Vzc::Syntax::SyntaxError e) {
			msgPrintf("Error at %zu, %zu: %s\n", e.sym->line, e.sym->column, e.what());
			return -1;
		}
	} catch (std::bad_alloc) {
		msgPuts("Out of memory");
		return ENOMEM;
	}

	return 0;
}
