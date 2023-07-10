#include <slake/runtime.h>
#include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::ValueRef<> print(slake::Runtime *rt, uint8_t nArgs, slake::ValueRef<> *args) {
	using namespace slake;

	for(uint8_t i=0;i<nArgs;++i) {
		switch(args[i]->getType().valueType) {
			case ValueType::I8:
				printf("%hhd", ((I8Value*)*args[i])->getData());
				break;
			case ValueType::I16:
				printf("%hd", ((I16Value*)*args[i])->getData());
				break;
			case ValueType::I32:
				printf("%d", ((I32Value*)*args[i])->getData());
				break;
			case ValueType::I64:
				printf("%lld", ((I64Value*)*args[i])->getData());
				break;
			case ValueType::U8:
				printf("%hhu", ((U8Value*)*args[i])->getData());
				break;
			case ValueType::U16:
				printf("%hu", ((U16Value*)*args[i])->getData());
				break;
			case ValueType::U32:
				printf("%u", ((U32Value*)*args[i])->getData());
				break;
			case ValueType::U64:
				printf("%llu", ((U64Value*)*args[i])->getData());
				break;
			case ValueType::F32:
				std::cout<<((F32Value*)*args[i])->getData();
				break;
			case ValueType::F64:
				std::cout<<((F64Value*)*args[i])->getData();
				break;
			case ValueType::BOOL:
				fputs(((BoolValue*)*args[i])->getData() ? "true" : "false" ,stdout);
				break;
			case ValueType::STRING:
				fputs(((StringValue*)*args[i])->getData().c_str(), stdout);
				break;
			default:
				throw std::runtime_error("In*args[i]alid argument type");
		}
	}

	return {};
}

slake::ValueRef<slake::ModuleValue> fsModuleLoader(slake::Runtime *rt, slake::RefValue *ref) {
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

void printTraceback(slake::Runtime *rt) {
	auto ctxt = rt->currentContexts.at(std::this_thread::get_id());
	printf("Traceback:\n");
	for (auto i = ctxt->majorFrames.rbegin(); i != ctxt->majorFrames.rend(); ++i) {
		printf("\t%s: 0x%08x\n", rt->resolveName(*(i->curFn)).c_str(), i->curIns);
	}
}

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

	std::unique_ptr<slake::Runtime> rt = std::make_unique<slake::Runtime>(slake::RT_DEBUG | slake::RT_GCDBG);

	slake::ValueRef<slake::ModuleValue> mod;
	try {
		auto fs = std::ifstream();
		fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
		fs.open("main.slx", std::ios_base::in | std::ios_base::binary);

		rt->setModuleLoader(fsModuleLoader);
		slake::stdlib::load(rt.get());

		mod = rt->loadModule(fs, "main");
		rt->getRootValue()->addMember("main", *mod);
	} catch (std::ios::failure e) {
		printf("Error loading main module\n");
		return -1;
	}

	rt->getRootValue()->addMember(
		"print",
		new slake::NativeFnValue(
			rt.get(),
			print,
			slake::ACCESS_PUB,
			slake::ValueType::NONE));

	slake::ValueRef<> result;

	try {
		rt->getRootValue()->getMember("main")->getMember("main")->call(0, nullptr);
	} catch (slake::NotFoundError e) {
		printf("NotFoundError: %s, ref = %s\n", e.what(), std::to_string(*e.ref).c_str());
		printTraceback(rt.get());
	} catch (slake::RuntimeExecError e) {
		auto ctxt = rt->currentContexts.at(std::this_thread::get_id());
		printf("RuntimeExecError: %s\n", e.what());
		printTraceback(rt.get());
	}

	result.release();
	mod.release();

	rt.release();
	return 0;
}
