#include <slake/runtime.h>
#include <slake/loader/loader.h>
#include <slake/opti/proganal.h>
// #include <slake/lib/std.h>

#include <iostream>

using namespace slake;

Value print(Context *context, MajorFrame *curMajorFrame) {
	if (curMajorFrame->resumableContextData->argStack.size() < 1)
		putchar('\n');
	else {
		Value varArgsValue;
		curMajorFrame->curFn->associatedRuntime->readVar(Reference::makeArgRef(curMajorFrame, 0), varArgsValue);

		for (uint8_t i = 0; i < curMajorFrame->resumableContextData->argStack.size(); ++i) {
			const Value &data = curMajorFrame->resumableContextData->argStack.at(i);

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
				case ValueType::Reference: {
					Object *objectPtr = data.getReference().asObject;
					if (!objectPtr)
						fputs("null", stdout);
					else {
						switch (objectPtr->getObjectKind()) {
							case ObjectKind::String:
								std::cout << ((StringObject *)objectPtr)->data.data();
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

void printTraceback(Runtime *rt, ContextObject *context) {
	printf("Traceback:\n");

	auto walker = [](MajorFrame *i, void *userData) {
		if (!i->curFn) {
			printf("(Stack top)\n");
			return true;
		}

		peff::DynArray<IdRefEntry> fullRef(peff::getDefaultAlloc());

		if (!i->associatedRuntime->getFullRef(peff::getDefaultAlloc(), i->curFn->fnObject, fullRef)) {
			throw std::bad_alloc();
		}

		std::string name;

		for (size_t i = 0; i < fullRef.size(); ++i) {
			if (i) {
				name += '.';
			}

			IdRefEntry &id = fullRef.at(i);

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

		printf("\t%s: %u", name.c_str(), i->resumableContextData->curIns);
		putchar('\n');

		return true;
	};

	context->getContext().forEachMajorFrame(walker, nullptr);
}

class MyReader : public loader::Reader {
public:
	peff::RcObjectPtr<peff::Alloc> selfAllocator;

	FILE *fp;

	MyReader(peff::Alloc *selfAllocator, FILE *fp) : selfAllocator(selfAllocator), fp(fp) {}
	virtual ~MyReader() {
		fclose(fp);
	}

	virtual bool isEof() noexcept {
		return feof(fp);
	}

	virtual loader::ReadResult read(char *buffer, size_t size) noexcept {
		if (fread(buffer, size, 1, fp) < 1) {
			return loader::ReadResult::ReadError;
		}
		return loader::ReadResult::Succeeded;
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

	// std::map<void *, AllocRecord> allocRecords;

	~MyAllocator() {
		//assert(allocRecords.empty());
	}

	virtual void *alloc(size_t size, size_t alignment) noexcept override {
		void *p = this->StdAlloc::alloc(size, alignment);
		if (!p)
			std::terminate();

		// allocRecords[p] = { size, alignment };

		return p;
	}

	virtual void *realloc(void *p, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept override {
		void *ptr = this->StdAlloc::realloc(p, size, alignment, newSize, newAlignment);
		if (!ptr)
			return nullptr;

		/* AllocRecord &allocRecord = allocRecords.at(p);

		assert(allocRecord.size == size);
		assert(allocRecord.alignment == alignment);

		allocRecords.erase(p);

		allocRecords[ptr] = { newSize, newAlignment };*/

		return ptr;
	}

	virtual void release(void *p, size_t size, size_t alignment) noexcept override {
		/* AllocRecord &allocRecord = allocRecords.at(p);

		assert(allocRecord.size == size);
		assert(allocRecord.alignment == alignment);

		allocRecords.erase(p);*/

		this->StdAlloc::release(p, size, alignment);
	}
};

class LoaderContext : public loader::LoaderContext {
public:
	LoaderContext(peff::Alloc *allocator) : loader::LoaderContext(allocator) {
	}
	~LoaderContext() {
	}

	virtual InternalExceptionPointer locateModule(Runtime *rt, const peff::DynArray<IdRefEntry> &ref, loader::Reader *&readerOut) {
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
			return BadMagicError::alloc(rt->getFixedAlloc());
		}

		peff::ScopeGuard closeFpGuard([fp]() noexcept {
			fclose(fp);
		});

		std::unique_ptr<MyReader, peff::DeallocableDeleter<MyReader>> reader(peff::allocAndConstruct<MyReader>(allocator.get(), alignof(MyReader), allocator.get(), fp));

		if (!reader)
			return OutOfMemoryError::alloc();

		readerOut = reader.release();

		closeFpGuard.release();

		return {};
	}
};

int main(int argc, char **argv) {
	util::setupMemoryLeakDetector();

	MyAllocator myAllocator;

	{
		std::unique_ptr<Runtime, peff::DeallocableDeleter<Runtime>> rt = std::unique_ptr<Runtime, peff::DeallocableDeleter<Runtime>>(
			Runtime::alloc(
				&myAllocator,
				&myAllocator,
				RT_DEBUG | RT_GCDBG));

		{
			HostObjectRef<ModuleObject> mod;
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

				if (auto e = loader::loadModule(loaderContext, rt.get(), &reader, mod); e) {
					printf("Error loading main module: %s\n", e->what());
					e.reset();
					return -1;
				}
			}

			{
				HostRefHolder hostRefHolder(rt->getFixedAlloc());

				HostObjectRef<ModuleObject> modObjectHostext = mod;
				HostObjectRef<ModuleObject> modObjectExtfns = ModuleObject::alloc(rt.get());

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

				HostObjectRef<FnObject> fnObject = FnObject::alloc(rt.get());

				auto printFn = NativeFnOverloadingObject::alloc(
					fnObject.get(),
					print);
				printFn->setAccess(ACCESS_PUBLIC);
				printFn->returnType = TypeId::Void;
				printFn->setVarArgs();
				printFn->overridenType = TypeId::Void;
				if (!fnObject->overloadings.insert({ printFn->paramTypes, printFn->isWithVarArgs(), printFn->genericParams.size(), printFn->overridenType }, printFn.get()))
					throw std::bad_alloc();
				fnObject->setName("print");

				((ModuleObject *)((ModuleObject *)rt->getRootObject()->getMember("hostext").asObject)
						->getMember("extfns")
						.asObject)
					->removeMember("print");
				if (!((ModuleObject *)((ModuleObject *)rt->getRootObject()->getMember("hostext").asObject)
							->getMember("extfns")
							.asObject)
						->addMember(fnObject.get()))
					throw std::bad_alloc();

				auto fn = (FnObject *)mod->getMember("main").asObject;
				FnOverloadingObject *overloading;

				peff::DynArray<TypeRef> params(&myAllocator);

				overloading = fn->overloadings.at(FnSignature(params, false, 0, TypeId::Void));

				/* opti::ProgramAnalyzedInfo analyzedInfo(rt.get(), &myAllocator);
				if (auto e = opti::analyzeProgramInfoPass(rt.get(), &myAllocator, (RegularFnOverloadingObject *)overloading, analyzedInfo, hostRefHolder);
					e) {
					printf("Internal exception: %s\n", e->what());
					switch (e->kind) {
						case ErrorKind::OptimizerError: {
							OptimizerError *err = (OptimizerError *)e.get();

							switch (err->optimizerErrorCode) {
								case OptimizerErrorCode::MalformedProgram: {
									MalformedProgramError *err = (MalformedProgramError *)e.get();

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
					printf("Lifetime: %u-%u\n", it.value().lifetime.offBeginIns, it.value().lifetime.offEndIns);
				}*/

				{
					HostObjectRef<CoroutineObject> co;
					if (auto e = rt->createCoroutineInstance(overloading, nullptr, nullptr, 0, co); e) {
						printf("Internal exception: %s\n", e->what());
						e.reset();
						goto end;
					}

					HostObjectRef<ContextObject> context;

					if (!(context = ContextObject::alloc(rt.get(), 114514))) {
						puts("Out of memory");
						goto end;
					}

					Value result;
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
		}

		rt.reset();
	}

	return 0;
}
