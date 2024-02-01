#include "../compiler.h"

using namespace slake::slkc;

string Compiler::mangleName(
	string name,
	const deque<shared_ptr<TypeNameNode>> &argTypes,
	bool isConst) {
	std::string s = name;

	if (isConst)
		s += "$const";

	for (auto i : argTypes) {
		s += "$" + std::to_string(i, this);
	}

	return s;
}
