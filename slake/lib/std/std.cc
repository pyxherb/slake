#include <slake/lib/std.h>
#include <slake/lib/core.h>
#include <slake/lib/std/util.h>
#include <slake/lib/std/math.h>

using namespace slake;

ModuleValue *stdlib::modStd;

void stdlib::load(Runtime *rt) {
	auto root = rt->getRootValue();

	root->scope->addMember("std", modStd = new ModuleValue(rt, ACCESS_PUB));
	math::load(rt);
	util::load(rt);
}
