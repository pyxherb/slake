#include <slake/runtime.h>

#include <fstream>
#include <cassert>

int main(int argc, char **argv) {
	Slake::Util::setupMemoryLeakDetector();
	Slake::Runtime *rt = new Slake::Runtime();
	auto fs = std::ifstream();
	fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	fs.open("main.slx", std::ios_base::in | std::ios_base::binary);
	rt->loadModule("main", fs);
	auto v = rt->getRootValue()->getMember("main")->getMember("main")->call(0, nullptr);
	assert(v->getType().valueType == Slake::ValueType::I32);
	printf("%d\n",((Slake::ValueRef<Slake::I32Value>)v)->getValue());
	v.release();
	delete rt;
	return 0;
}
