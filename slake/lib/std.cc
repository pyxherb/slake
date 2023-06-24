#include "std.h"
#include "core.h"
#include "util.h"

using namespace Slake;

ModuleValue *StdLib::modStd;

void StdLib::load(Runtime *rt) {
	auto root = rt->getRootValue();

	root->addMember("std", modStd = new ModuleValue(rt, ACCESS_PUB));
	Core::load(rt);
}
