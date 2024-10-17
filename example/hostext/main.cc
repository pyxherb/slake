#include <slake/runtime.h>
// #include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::Value print(
	slake::NativeFnOverloadingObject *overloading,
	slake::Object *thisObject,
	slake::RegularVarObject **args,
	size_t nArgs) {
	using namespace slake;

	if (nArgs < 1)
		putchar('\n');
	else {
		AnyArrayObject *varArgs = (AnyArrayObject *)args[0]->getData({}).getObjectRef();

		for (uint8_t i = 0; i < nArgs; ++i) {
			auto data = varArgs->accessor->getData(VarRefContext::makeArrayContext(i));

			switch (data.valueType) {
				case ValueType::I8:
					std::cout << data.getI8();
					break;
				case ValueType::I16:
					std::cout << data.getI16();
					break;
				case ValueType::I32:
					std::cout << data.getI32();
					break;
				case ValueType::I64:
					std::cout << data.getI64();
					break;
				case ValueType::U8:
					std::cout << data.getU8();
					break;
				case ValueType::U16:
					std::cout << data.getU16();
					break;
				case ValueType::U32:
					std::cout << data.getU32();
					break;
				case ValueType::U64:
					std::cout << data.getU64();
					break;
				case ValueType::F32:
					std::cout << data.getF32();
					break;
				case ValueType::F64:
					std::cout << data.getF64();
					break;
				case ValueType::Bool:
					fputs(data.getBool() ? "true" : "false", stdout);
					break;
				case ValueType::ObjectRef: {
					Object *objectPtr = data.getObjectRef();
					if (!objectPtr)
						fputs("null", stdout);
					else {
						switch (objectPtr->getKind()) {
							case ObjectKind::String:
								std::cout << ((slake::StringObject *)objectPtr)->data;
								break;
							default:
								std::cout << "<object at " << std::hex << objectPtr << ">";
								break;
						}
					}
					break;
				}
				default:
					throw std::runtime_error("Invalid argument type");
			}
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
	for (auto i = ctxt->getContext().majorFrames.rbegin(); i != ctxt->getContext().majorFrames.rend(); ++i) {
		printf("\t%s: 0x%08x", rt->getFullName((*i)->curFn->fnObject).c_str(), (*i)->curIns);
		switch ((*i)->curFn->getOverloadingKind()) {
			case slake::FnOverloadingKind::Regular: {
				if (auto sld = ((slake::RegularFnOverloadingObject *)(*i)->curFn)->getSourceLocationDesc((*i)->curIns); sld) {
					printf(" at %d:%d", sld->line, sld->column);
				}
				break;
			}
			default:;
		}
		putchar('\n');
	}
}

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

	std::unique_ptr<slake::Runtime> rt = std::make_unique<slake::Runtime>(
		std::pmr::get_default_resource(),
		slake::RT_DEBUG | slake::RT_GCDBG);

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

		std::vector<slake::Type> paramTypes;

		auto printFn = slake::NativeFnOverloadingObject::alloc(
			fnObject.get(),
			slake::ACCESS_PUB,
			paramTypes,
			slake::ValueType::Undefined,
			print);
		printFn->overloadingFlags |= slake::OL_VARG;
		fnObject->overloadings.insert(printFn.get());

		((slake::ModuleObject *)((slake::ModuleObject *)rt->getRootObject()->getMember("hostext", nullptr))->getMember("extfns", nullptr))->scope->putMember("print", fnObject.get());

		try {
			auto fn = (slake::FnObject *)mod->getMember("main", nullptr);

			slake::HostObjectRef<slake::ContextObject> context =
				rt->execFn(fn->getOverloading({}), nullptr, nullptr, nullptr, 0);
			printf("%d\n", context->getResult().getI32());

			while (!context->isDone()) {
				context->resume(&hostRefHolder);

				printf("%d\n", context->getResult().getI32());
			}

			puts("");
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
