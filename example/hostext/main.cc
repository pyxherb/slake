#include <slake/runtime.h>
#include <slake/opti/proganal.h>
// #include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::Value print(slake::Context *context, slake::MajorFrame *curMajorFrame) {
	using namespace slake;

	if (curMajorFrame->argStack.size() < 1)
		putchar('\n');
	else {
		Value varArgsValue;
		varArgsValue = curMajorFrame->curFn->associatedRuntime->readVarUnsafe(slake::EntityRef::makeArgRef(curMajorFrame, 0));
		ArrayObject *varArgs = (ArrayObject *)varArgsValue.getEntityRef().asObject.instanceObject;

		for (uint8_t i = 0; i < varArgs->length; ++i) {
			Value data;
			if (auto e = curMajorFrame->curFn->associatedRuntime->readVar(
					EntityRef::makeArrayElementRef(varArgs, i),
					data)) {
				throw std::runtime_error("An exception has thrown");
			}

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
			case ValueType::EntityRef: {
				Object *objectPtr = data.getEntityRef().asObject.instanceObject;
				if (!objectPtr)
					fputs("null", stdout);
				else {
					switch (objectPtr->getKind()) {
					case ObjectKind::String:
						std::cout << ((slake::StringObject *)objectPtr)->data.data();
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

std::unique_ptr<std::istream> fsModuleLocator(slake::Runtime *rt, const peff::DynArray<slake::IdRefEntry> &entries) {
	std::string path;
	for (size_t i = 0; i < entries.size(); ++i) {
		path += entries.at(i).name;
		if (i + 1 < entries.size())
			path += "/";
	}
	path += ".slx";

	std::unique_ptr<std::ifstream> fs = std::make_unique<std::ifstream>();
	fs->exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	fs->open(path, std::ios_base::binary);

	return fs;
}

void printTraceback(slake::Runtime *rt, slake::ContextObject *context) {
	printf("Traceback:\n");
	for (auto i = context->getContext().majorFrameList;
		 i;
		 i = i->next) {
		if (!i->curFn) {
			printf("(Stack top)\n");
			continue;
		}
		printf("\t%s: 0x%08x", rt->getFullName(i->curFn->fnObject).c_str(), i->curIns);
		switch (i->curFn->overloadingKind) {
		case slake::FnOverloadingKind::Regular: {
			if (auto sld = ((slake::RegularFnOverloadingObject *)i->curFn)->getSourceLocationDesc(i->curIns); sld) {
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
		peff::getDefaultAlloc(),
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
		slake::HostRefHolder hostRefHolder(&rt->globalHeapPoolAlloc);

		slake::HostObjectRef<slake::FnObject> fnObject = slake::FnObject::alloc(rt.get());

		peff::DynArray<slake::Type> paramTypes(&rt->globalHeapPoolAlloc);

		auto printFn = slake::NativeFnOverloadingObject::alloc(
			fnObject.get(),
			slake::ACCESS_PUB,
			std::move(paramTypes),
			slake::TypeId::None,
			slake::OL_VARG,
			print);
		if (!fnObject->overloadings.insert(printFn.get()))
			throw std::bad_alloc();
		fnObject->name.resize(strlen("print"));
		memcpy(fnObject->name.data(), "print", strlen("print"));

		((slake::ModuleObject *)((slake::ModuleObject *)rt->getRootObject()->getMember("hostext").asObject.instanceObject)
				->getMember("extfns")
				.asObject.instanceObject)
			->scope->removeMember("print");
		if (!((slake::ModuleObject *)((slake::ModuleObject *)rt->getRootObject()->getMember("hostext").asObject.instanceObject)
					->getMember("extfns")
					.asObject.instanceObject)
				 ->scope->putMember(fnObject.get()))
			throw std::bad_alloc();

		auto fn = (slake::FnObject *)mod->getMember("main").asObject.instanceObject;
		auto overloading = fn->getOverloading({});

		slake::opti::ProgramAnalyzedInfo analyzedInfo(rt.get());
		if (auto e = slake::opti::analyzeProgramInfo(rt.get(), (slake::RegularFnOverloadingObject *)overloading, analyzedInfo, hostRefHolder);
			e) {
			printf("Internal exception: %s\n", e->what());
			switch (e->kind) {
			case slake::ErrorKind::OptimizerError: {
				slake::OptimizerError *err = (slake::OptimizerError *)e.get();

				switch (err->optimizerErrorCode) {
				case slake::OptimizerErrorCode::MalformedProgram: {
					slake::MalformedProgramError *err = (slake::MalformedProgramError *)e.get();

					printf("Malformed program error at instruction #%zu\n", err->offIns);
				}
				default:;
				}
			}
			default:;
			}
			e.reset();
			goto end;
		}
		for (auto it = analyzedInfo.analyzedRegInfo.begin(); it != analyzedInfo.analyzedRegInfo.end(); ++it) {
			printf("Register #%u\n", it.key());
			printf("Lifetime: %zu-%zu\n", it.value().lifetime.offBeginIns, it.value().lifetime.offEndIns);
		}

		slake::HostObjectRef<slake::ContextObject> context;
		if (auto e = rt->execFn(overloading, nullptr, nullptr, nullptr, 0, context);
			e) {
			printf("Internal exception: %s\n", e->what());
			printTraceback(rt.get(), context.get());
			e.reset();
			goto end;
		}
		printf("%d\n", context->getResult().getI32());

		while (!context->isDone()) {
			if (auto e = context->resume(&hostRefHolder);
				e) {
				printf("Internal exception: %s\n", e->what());
				printTraceback(rt.get(), context.get());
				e.reset();
				goto end;
			}

			printf("%d\n", context->getResult().getI32());
		}

		puts("");
	}
end:

	mod.reset();

	rt.reset();
	return 0;
}
