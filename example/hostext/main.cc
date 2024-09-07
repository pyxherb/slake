#include <slake/runtime.h>
// #include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::Value print(
	slake::Runtime *rt,
	slake::Object *thisObject,
	std::deque<slake::Value> args,
	const std::unordered_map<std::string, slake::Type> &mappedGenericArgs) {
	using namespace slake;

	for (uint8_t i = 0; i < args.size(); ++i) {
		switch (args[i].valueType) {
			case ValueType::I8:
				std::cout << args[i].getI8();
				break;
			case ValueType::I16:
				std::cout << args[i].getI16();
				break;
			case ValueType::I32:
				std::cout << args[i].getI32();
				break;
			case ValueType::I64:
				std::cout << args[i].getI64();
				break;
			case ValueType::U8:
				std::cout << args[i].getU8();
				break;
			case ValueType::U16:
				std::cout << args[i].getU16();
				break;
			case ValueType::U32:
				std::cout << args[i].getU32();
				break;
			case ValueType::U64:
				std::cout << args[i].getU64();
				break;
			case ValueType::F32:
				std::cout << args[i].getF32();
				break;
			case ValueType::F64:
				std::cout << args[i].getF64();
				break;
			case ValueType::Bool:
				fputs(args[i].getBool() ? "true" : "false", stdout);
				break;
			case ValueType::ObjectRef: {
				const ObjectRefValueExData &exData = args[i].getObjectRef();
				if (!exData.objectPtr)
					fputs("null", stdout);
				else {
					switch (exData.objectPtr->getKind()) {
						case ObjectKind::String:
							std::cout << ((slake::StringObject *)exData.objectPtr)->data;
							break;
						default:
							std::cout << "<object at " << std::hex << exData.objectPtr << ">";
							break;
					}
				}
				break;
			}
			default:
				throw std::runtime_error("Invalid argument type");
		}
	}

	return {};
}

std::unique_ptr<std::istream> fsModuleLocator(slake::Runtime *rt, slake::HostObjectRef<slake::IdRefObject> ref) {
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
		printf("\t%s: 0x%08x", rt->getFullName((*i)->curFn->fnObject).c_str(), (*i)->curIns);
		putchar('\n');
	}
}

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

	std::unique_ptr<slake::Runtime> rt = std::make_unique<slake::Runtime>(slake::RT_DEBUG | slake::RT_GCDBG);

	slake::HostObjectRef<slake::ModuleObject> mod;
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

	{
		slake::HostRefHolder hostRefHolder;

		slake::HostObjectRef<slake::FnObject> fnObject = slake::FnObject::alloc(rt.get());

		fnObject->overloadings.insert(
			slake::NativeFnOverloadingObject::alloc(fnObject.get(), slake::ACCESS_PUB, std::deque<slake::Type>{}, slake::ValueType::Undefined, print).release());

		((slake::ModuleObject *)((slake::ModuleObject *)rt->getRootObject()->getMember("hostext", nullptr))->getMember("extfns", nullptr))->scope->putMember("print", fnObject.get());

		try {
			slake::Value result =
				((slake::FnObject *)mod->getMember("main", nullptr))->call(nullptr, {}, {}, &hostRefHolder);

			slake::HostObjectRef<slake::ContextObject> context = (slake::ContextObject *)result.getObjectRef().objectPtr;
			printf("%d\n", context->getResult().getI32());

			while (!context->isDone()) {
				context->resume(&hostRefHolder);

				printf("%d\n", context->getResult().getI32());
			}
		} catch (slake::NotFoundError e) {
			printf("NotFoundError: %s, ref = %s\n", e.what(), std::to_string(e.ref).c_str());
			printTraceback(rt.get());
		} catch (slake::RuntimeExecError e) {
			auto ctxt = rt->activeContexts.at(std::this_thread::get_id());
			printf("RuntimeExecError: %s\n", e.what());
			printTraceback(rt.get());
		}
	}

	mod.reset();

	rt.reset();
	return 0;
}
