#include "std.h"
#include "core.h"
#include "util.h"

using namespace slake;

ModuleValue *stdlib::modStd;

void stdlib::load(Runtime *rt) {
	auto root = rt->getRootValue();

	root->addMember("std", modStd = new ModuleValue(rt, ACCESS_PUB));
	//Core::load(rt);
	util::load(rt);
}
