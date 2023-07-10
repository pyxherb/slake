#include "util.h"
#include "std.h"

using namespace slake;

void stdlib::util::load(Runtime *rt) {
	auto root = rt->getRootValue();

	modStd->addMember(
		"util",
		modUtil = new ModuleValue(rt, ACCESS_PUB));
	Math::load(rt);
}
