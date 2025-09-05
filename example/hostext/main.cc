#include <slake/runtime.h>
#include <slake/loader/loader.h>
// #include <slake/opti/proganal.h>
// #include <slake/lib/std.h>

#include <cassert>
#include <fstream>
#include <iostream>

slake::Value print(slake::Context *context, slake::MajorFrame *curMajorFrame) {
	using namespace slake;

	if (curMajorFrame->resumable->argStack.size() < 1)
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
						switch (objectPtr->getObjectKind()) {
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

void printTraceback(slake::Runtime *rt, slake::ContextObject *context) {
	printf("Traceback:\n");
	for (auto &i : context->_context.majorFrames) {
		if (!i->curFn) {
			printf("(Stack top)\n");
			continue;
		}

		peff::DynArray<slake::IdRefEntry> fullRef(peff::getDefaultAlloc());

		if (!rt->getFullRef(peff::getDefaultAlloc(), i->curFn->fnObject, fullRef)) {
			throw std::bad_alloc();
		}

		std::string name;

		for (size_t i = 0; i < fullRef.size(); ++i) {
			if (i) {
				name += '.';
			}

			slake::IdRefEntry &id = fullRef.at(i);

			name += id.name;

			if (id.genericArgs.size()) {
				name += '<';

				/* for (size_t j = 0; j < id.genericArgs.size(); ++j) {
					if (j)
						name += ",";
					name += std::to_string(id.genericArgs.at(j), rt);
				}*/

				name += '>';
			}
		}

		printf("\t%s: 0x%08x", name.c_str(), i->resumable->curIns);
		switch (i->curFn->overloadingKind) {
			case slake::FnOverloadingKind::Regular: {
				if (auto sld = ((slake::RegularFnOverloadingObject *)i->curFn)->getSourceLocationDesc(i->resumable->curIns); sld) {
					printf(" at %d:%d", sld->line, sld->column);
				}
				break;
			}
			default:;
		}
		putchar('\n');
	}
}

class MyReader : public slake::loader::Reader {
public:
	peff::RcObjectPtr<peff::Alloc> selfAllocator;

	FILE *fp;

	MyReader(peff::Alloc *selfAllocator, FILE *fp) : fp(fp) {}
	virtual ~MyReader() {
	}

	virtual bool isEof() noexcept {
		return feof(fp);
	}

	virtual slake::loader::ReadResult read(char *buffer, size_t size) noexcept {
		if (fread(buffer, size, 1, fp) < 1) {
			return slake::loader::ReadResult::ReadError;
		}
		return slake::loader::ReadResult::Succeeded;
	}

	virtual void dealloc() noexcept override {
		peff::destroyAndRelease<MyReader>(selfAllocator.get(), this, alignof(MyReader));
	}
};

class MyAllocator : public peff::StdAlloc {
public:
	struct AllocRecord {
		size_t size;
		size_t alignment;
	};

	std::map<void *, AllocRecord> allocRecords;

	~MyAllocator() {
		assert(allocRecords.empty());
	}

	virtual void *alloc(size_t size, size_t alignment) noexcept override {
		void *p = this->StdAlloc::alloc(size, alignment);
		if (!p)
			std::terminate();

		allocRecords[p] = { size, alignment };

		return p;
	}

	virtual void release(void *p, size_t size, size_t alignment) noexcept override {
		AllocRecord &allocRecord = allocRecords.at(p);

		assert(allocRecord.size == size);
		assert(allocRecord.alignment == alignment);

		allocRecords.erase(p);

		this->StdAlloc::release(p, size, alignment);
	}
};

class LoaderContext : public slake::loader::LoaderContext {
public:
	LoaderContext(peff::Alloc *allocator) : slake::loader::LoaderContext(allocator) {
	}
	~LoaderContext() {
	}

	virtual slake::InternalExceptionPointer locateModule(slake::Runtime *rt, const peff::DynArray<slake::IdRefEntry> &ref, slake::loader::Reader *&readerOut) {
		std::string path;
		for (size_t i = 0; i < ref.size(); ++i) {
			path += ref.at(i).name;
			if (i + 1 < ref.size())
				path += "/";
		}
		path += ".slx";

		FILE *fp;

		if (!(fp = fopen(path.c_str(), "rb"))) {
			puts("Error opening the main module");
			return slake::BadMagicError::alloc(rt->getFixedAlloc());
		}

		peff::ScopeGuard closeFpGuard([fp]() noexcept {
			fclose(fp);
		});

		std::unique_ptr<MyReader, peff::DeallocableDeleter<MyReader>> reader(peff::allocAndConstruct<MyReader>(allocator.get(), alignof(MyReader), allocator.get(), fp));

		if (!reader)
			return slake::OutOfMemoryError::alloc();

		readerOut = reader.release();

		return {};
	}
};

int main(int argc, char **argv) {
	slake::util::setupMemoryLeakDetector();

	MyAllocator myAllocator;

	{
		std::unique_ptr<slake::Runtime, peff::DeallocableDeleter<slake::Runtime>> rt = std::unique_ptr<slake::Runtime, peff::DeallocableDeleter<slake::Runtime>>(
			slake::Runtime::alloc(
				&myAllocator,
				&myAllocator,
				slake::RT_DEBUG | slake::RT_GCDBG));

		slake::HostObjectRef<slake::ModuleObject> mod;
		{
			FILE *fp;

			if (!(fp = fopen("hostext/main.slx", "rb"))) {
				puts("Error opening the main module");
				return -1;
			}

			peff::ScopeGuard closeFpGuard([fp]() noexcept {
				fclose(fp);
			});

			LoaderContext loaderContext(peff::getDefaultAlloc());
			MyReader reader(&peff::g_nullAlloc, fp);

			slake::loader::loadModule(loaderContext, rt.get(), &reader, mod);
		}

		{
			slake::HostRefHolder hostRefHolder(rt->getFixedAlloc());

			slake::HostObjectRef<slake::ModuleObject> modObjectHostext = mod;
			slake::HostObjectRef<slake::ModuleObject> modObjectExtfns = slake::ModuleObject::alloc(rt.get());

			/*
			if (!modObjectHostext->setName("hostext")) {
				std::terminate();
			}*/
			if (!modObjectExtfns->setName("extfns")) {
				std::terminate();
			}

			if (!modObjectHostext->addMember(modObjectExtfns.get())) {
				std::terminate();
			}
			if (!rt->getRootObject()->addMember(modObjectHostext.get())) {
				std::terminate();
			}

			slake::HostObjectRef<slake::FnObject> fnObject = slake::FnObject::alloc(rt.get());

			auto printFn = slake::NativeFnOverloadingObject::alloc(
				fnObject.get(),
				print);
			printFn->setAccess(slake::ACCESS_PUB);
			printFn->returnType = slake::TypeId::Void;
			printFn->setVarArgs();
			if (!fnObject->overloadings.insert(printFn.get()))
				throw std::bad_alloc();
			fnObject->name.build("print");

			((slake::ModuleObject *)((slake::ModuleObject *)rt->getRootObject()->getMember("hostext").asObject.instanceObject)
					->getMember("extfns")
					.asObject.instanceObject)
				->removeMember("print");
			if (!((slake::ModuleObject *)((slake::ModuleObject *)rt->getRootObject()->getMember("hostext").asObject.instanceObject)
						->getMember("extfns")
						.asObject.instanceObject)
					->addMember(fnObject.get()))
				throw std::bad_alloc();

			auto fn = (slake::FnObject *)mod->getMember("main").asObject.instanceObject;
			slake::FnOverloadingObject *overloading;

			if (fn->getOverloading(&myAllocator, peff::DynArray<slake::TypeRef>(&myAllocator), overloading)) {
				throw std::bad_alloc();
			}

			/*
			slake::opti::ProgramAnalyzedInfo analyzedInfo(rt.get(), &myAllocator);
			if (auto e = slake::opti::analyzeProgramInfo(rt.get(), &myAllocator, (slake::RegularFnOverloadingObject *)overloading, analyzedInfo, hostRefHolder);
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
			}*/

			{
				slake::HostObjectRef<slake::CoroutineObject> co;
				if (auto e = rt->createCoroutineInstance(overloading, nullptr, nullptr, 0, co); e) {
					printf("Internal exception: %s\n", e->what());
					e.reset();
					goto end;
				}

				slake::HostObjectRef<slake::ContextObject> context;

				if (!(context = slake::ContextObject::alloc(rt.get()))) {
					puts("Out of memory");
					goto end;
				}

				slake::Value result;
				while (!co->isDone()) {
					if (auto e = rt->resumeCoroutine(context.get(), co.get(), result);
						e) {
						printf("Internal exception: %s\n", e->what());
						printTraceback(rt.get(), context.get());
						e.reset();
						goto end;
					}
				}
			}
			rt->gc();

			puts("");
		}
	end:

		mod.reset();

		rt.reset();
	}

	return 0;
}
