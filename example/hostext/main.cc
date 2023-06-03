#include <slake/runtime.h>

#include <cassert>
#include <fstream>

Slake::ValueRef<> println(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args) {
	using namespace Slake;
	StringValue *v = (StringValue *)*(args[0]);
	if (v->getType() != ValueType::STRING)
		throw std::runtime_error("Invalid argument type");
	puts(v->getValue().c_str());
	return Slake::ValueRef<>();
}

Slake::ValueRef<Slake::ModuleValue> fsModuleLoader(Slake::Runtime *rt, Slake::RefValue *ref) {
	std::string path;
	for (size_t i = 0; i < ref->scopes.size(); i++) {
		path += ref->scopes[i];
		if (i + 1 < ref->scopes.size())
			path += "/";
	}

	std::ifstream fs;
	fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	fs.open(path, std::ios_base::binary);

	auto mod = rt->loadModule(fs, ref->scopes.back());

	fs.close();

	return mod;
}

int main(int argc, char **argv) {
	Slake::Util::setupMemoryLeakDetector();
	Slake::Runtime *rt = new Slake::Runtime(Slake::RT_DEBUG | Slake::RT_GCDBG);

	auto fs = std::ifstream();
	fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	fs.open("main.slx", std::ios_base::in | std::ios_base::binary);

	rt->setModuleLoader(fsModuleLoader);

	rt->getRootValue()->addMember("main", *(rt->loadModule(fs, "main")));

	rt->getRootValue()->addMember(
		"println",
		new Slake::NativeFnValue(
			rt,
			println,
			Slake::ACCESS_PUB,
			Slake::ValueType::NONE,
			rt->getRootValue()));

	// std::printf("%s\n", std::to_string(rt).c_str());

	Slake::ValueRef<> v;

	v = rt->getRootValue()->getMember("main")->getMember("main")->call(0, nullptr);
	/*
	try {
		v = rt->getRootValue()->getMember("main")->getMember("main")->call(0, nullptr);
	} catch (...) {
		std::printf("Dumping state:\n%s\n", std::to_string(*rt).c_str());
		std::printf("Dumping context:\n%s\n", std::to_string(*(rt->threadCurrentContexts.at(std::this_thread::get_id()))).c_str());
		std::rethrow_exception(std::current_exception());
	}*/

	assert(v && v->getType().valueType == Slake::ValueType::I32);
	printf("%d\n", ((Slake::ValueRef<Slake::I32Value>)v)->getValue());

	v.release();

	delete rt;
	return 0;
}
