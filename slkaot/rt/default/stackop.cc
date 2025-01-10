#include "stackop.h"

using namespace slake;
using namespace slake::aot_rt;

void *slake::aot_rt::getStackPtr() {
	void *volatile p = NULL;
	return (void *)&p;
}
