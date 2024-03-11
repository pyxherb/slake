#include <slake/lib/std/util.h>
#include <slake/lib/std.h>

using namespace slake;

ModuleValue *stdlib::util::modUtil;

void stdlib::util::load(Runtime *rt) {
	auto root = rt->getRootValue();

	modStd->scope->addMember(
		"util",
		modUtil = new ModuleValue(rt, ACCESS_PUB));
}
