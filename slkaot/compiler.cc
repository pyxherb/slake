#include "compiler.h"

using namespace slake;
using namespace slake::slkaot;

std::vector<std::string> slkaot::modulePaths;

std::unique_ptr<std::ifstream> slkaot::moduleLocator(Runtime *rt, HostObjectRef<IdRefObject> ref) {
	std::string path;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		path += ref->entries.at(i).name;
		if (i + 1 < ref->entries.size())
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
