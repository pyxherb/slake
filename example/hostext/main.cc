#include <slake/runtime.h>
#include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::ValueRef<> print(
	slake::Runtime *rt,
	slake::Value *thisObject,
	std::deque<slake::Value *> args,
	const std::unordered_map<std::string, slake::Type> &mappedGenericArgs) {
	using namespace slake;

	for (uint8_t i = 0; i < args.size(); ++i) {
		if (!args[i]) {
			throw slake::NullRefError();
			continue;
		}

		switch (args[i]->getType().typeId) {
			case TypeId::I8:
				std::cout << ((I8Value *)args[i])->getData();
				break;
			case TypeId::I16:
				std::cout << ((I16Value *)args[i])->getData();
				break;
			case TypeId::I32:
				std::cout << ((I32Value *)args[i])->getData();
				break;
			case TypeId::I64:
				std::cout << ((I64Value *)args[i])->getData();
				break;
			case TypeId::U8:
				std::cout << ((U8Value *)args[i])->getData();
				break;
			case TypeId::U16:
				std::cout << ((U16Value *)args[i])->getData();
				break;
			case TypeId::U32:
				std::cout << ((U32Value *)args[i])->getData();
				break;
			case TypeId::U64:
				std::cout << ((U64Value *)args[i])->getData();
				break;
			case TypeId::F32:
				std::cout << ((F32Value *)args[i])->getData();
				break;
			case TypeId::F64:
				std::cout << ((F64Value *)args[i])->getData();
				break;
			case TypeId::Bool:
				fputs(((BoolValue *)args[i])->getData() ? "true" : "false", stdout);
				break;
			case TypeId::String:
				fputs(((StringValue *)args[i])->getData().c_str(), stdout);
				break;
			default:
				throw std::runtime_error("Invalid argument type");
		}
	}

	return {};
}

std::unique_ptr<std::istream> fsModuleLocator(slake::Runtime *rt, slake::ValueRef<slake::IdRefValue> ref) {
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
		printf("\t%s: 0x%08x", rt->getFullName(i->curFn->fnValue).c_str(), i->curIns);
		putchar('\n');
	}
}

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

	std::unique_ptr<slake::Runtime> rt = std::make_unique<slake::Runtime>(slake::RT_DEBUG | slake::RT_GCDBG);

	slake::ValueRef<slake::ModuleValue> mod;
	{
		std::ifstream fs;
		try {
			fs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
			fs.open("hostext/main.slx", std::ios_base::in | std::ios_base::binary);

			rt->setModuleLocator(fsModuleLocator);
			// slake::stdlib::load(rt.get());

			mod = rt->loadModule(fs, slake::LMOD_NOCONFLICT);
		} catch (slake::LoaderError e) {
			printf("Error loading main module: %s, at file offset %zu\n", e.what(), (size_t)fs.tellg());
			return -1;
		} catch (std::ios::failure e) {
			printf("Error loading main module: %s, at file offset %zu\n", e.what(), (size_t)fs.tellg());
			return -1;
		}
	}

	slake::ValueRef<slake::FnValue> fnValue = new slake::FnValue(rt.get());

	fnValue->overloadings.push_back(new slake::NativeFnOverloadingValue(fnValue.get(), slake::ACCESS_PUB, std::deque<slake::Type>{}, slake::TypeId::None, print));

	((slake::ModuleValue *)((slake::ModuleValue *)rt->getRootValue()->getMember("hostext"))->getMember("extfns"))->scope->putMember("print", fnValue.get());

	try {
		slake::ValueRef<slake::ContextValue> context = (slake::ContextValue *)(((slake::FnValue *)mod->scope->getMember("main"))->call(nullptr, {}, {}).get());
		printf("%d\n", ((slake::I32Value *)context->getResult().get())->getData());
		while (!context->isDone()) {
			context->resume();

			auto result = context->getResult();
			assert(result->getType() == slake::TypeId::I32);

			printf("%d\n", ((slake::I32Value *)result.get())->getData());
		}
	} catch (slake::NotFoundError e) {
		printf("NotFoundError: %s, ref = %s\n", e.what(), std::to_string(e.ref.get()).c_str());
		printTraceback(rt.get());
	} catch (slake::RuntimeExecError e) {
		auto ctxt = rt->activeContexts.at(std::this_thread::get_id());
		printf("RuntimeExecError: %s\n", e.what());
		printTraceback(rt.get());
	}

	fnValue.reset();

	mod.reset();

	rt.reset();
	return 0;
}
