#include <slake/runtime.h>
#include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::ValueRef<> print(slake::Runtime *rt, std::deque<slake::ValueRef<>> args) {
	using namespace slake;

	for (uint8_t i = 0; i < args.size(); ++i) {
		switch (args[i]->getType().typeId) {
			case TypeId::I8:
				printf("%hhd", ((I8Value *)*args[i])->getData());
				break;
			case TypeId::I16:
				printf("%hd", ((I16Value *)*args[i])->getData());
				break;
			case TypeId::I32:
				printf("%d", ((I32Value *)*args[i])->getData());
				break;
			case TypeId::I64:
				std::cout << ((I64Value *)*args[i])->getData();
				break;
			case TypeId::U8:
				printf("%hhu", ((U8Value *)*args[i])->getData());
				break;
			case TypeId::U16:
				printf("%hu", ((U16Value *)*args[i])->getData());
				break;
			case TypeId::U32:
				printf("%u", ((U32Value *)*args[i])->getData());
				break;
			case TypeId::U64:
				printf("%lu", ((U64Value *)*args[i])->getData());
				break;
			case TypeId::F32:
				std::cout << ((F32Value *)*args[i])->getData();
				break;
			case TypeId::F64:
				std::cout << ((F64Value *)*args[i])->getData();
				break;
			case TypeId::BOOL:
				fputs(((BoolValue *)*args[i])->getData() ? "true" : "false", stdout);
				break;
			case TypeId::STRING:
				fputs(((StringValue *)*args[i])->getData().c_str(), stdout);
				break;
			default:
				throw std::runtime_error("Invalid argument type");
		}
	}

	return {};
}

std::unique_ptr<std::istream> fsModuleLocator(slake::Runtime *rt, slake::ValueRef<slake::RefValue> ref) {
	std::string path;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		path += ref->entries[i].name;
		if (i + 1 < ref->entries.size())
			path += "/";
	}
	path += ".slx";

	std::unique_ptr<std::ifstream> fs = std::make_unique<std::ifstream>();
	fs->exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	fs->open(path, std::ios_base::binary);

	return fs;
}

void printTraceback(slake::Runtime *rt) {
	auto ctxt = rt->activeContexts.at(std::this_thread::get_id());
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

		rt->setModuleLocator(fsModuleLocator);
		slake::stdlib::load(rt.get());

		mod = rt->loadModule(fs, slake::LMOD_NOCONFLICT);
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
			slake::TypeId::NONE));

	slake::ValueRef<> result;

	try {
		slake::ValueRef<slake::ContextValue> context = (slake::ContextValue *)*(mod->getMember("main")->call({}));
		printf("%d\n", ((slake::I32Value *)*context->getResult())->getData());
		context->resume();
		printf("%d\n", ((slake::I32Value *)*context->getResult())->getData());
		context->resume();
		printf("%d\n", ((slake::I32Value *)*context->getResult())->getData());
		context->resume();
		printf("%d\n", ((slake::I32Value *)*context->getResult())->getData());
		context->resume();
		printf("%d\n", ((slake::I32Value *)*context->getResult())->getData());
	} catch (slake::NotFoundError e) {
		printf("NotFoundError: %s, ref = %s\n", e.what(), std::to_string(*e.ref).c_str());
		printTraceback(rt.get());
	} catch (slake::RuntimeExecError e) {
		auto ctxt = rt->activeContexts.at(std::this_thread::get_id());
		printf("RuntimeExecError: %s\n", e.what());
		printTraceback(rt.get());
	}

	result.release();
	mod.release();

	rt.reset();
	return 0;
}
