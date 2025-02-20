#include "compiler.h"

using namespace slake;
using namespace slake::slkaot;

std::vector<std::string> slkaot::modulePaths;
std::string slkaot::srcPath, slkaot::headerOutPath, slkaot::sourceOutPath, slkaot::includeName;

std::unique_ptr<std::istream> slkaot::moduleLocator(Runtime *rt, const peff::DynArray<IdRefEntry> &ref) {
	std::string path;
	for (size_t i = 0; i < ref.size(); ++i) {
		path += ref.at(i).name;
		if (i + 1 < ref.size())
			path += "/";
	}

	std::unique_ptr<std::ifstream> fs = std::make_unique<std::ifstream>();
	fs->exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	for (auto i : modulePaths) {
		try {
			fs->open(i + "/" + path + ".slx", std::ios_base::binary);
			return fs;
		} catch (std::ios::failure) {
			fs->clear();
		}
	}

	return nullptr;
}
