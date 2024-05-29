#include <slake/runtime.h>

using namespace slake;

std::string Runtime::mangleName(
	std::string name,
	std::deque<Type> params,
	GenericArgList genericArgs,
	bool isConst) const {
	std::string s = name;

	if (isConst)
		s += "$const";

	for (auto i : params)
		s += "$" + std::to_string(i, this);

	for (auto i : genericArgs)
		s += "!" + std::to_string(i, this);

	return s;
}
