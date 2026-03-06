#include <slake/runtime.h>
#include <slake/loader/loader.h>
#include <slake/opti/proganal.h>
// #include <slake/lib/std.h>

#include <iostream>

using namespace slake;

Value print(Context *context, MajorFrame *curMajorFrame) {
	if (curMajorFrame->resumableContextData.nArgs < 1)
		putchar('\n');
	else {
		for (uint8_t i = 0; i < curMajorFrame->resumableContextData.nArgs; ++i) {
			Value data;
			if (curMajorFrame->curCoroutine)
				Runtime::readVar(CoroutineArgRef(curMajorFrame->curCoroutine, i), data);
			else
				Runtime::readVar(ArgRef(curMajorFrame, i), data);

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

		printf("\t%s: %u", name.c_str(), i->resumableContextData.curIns);
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

	virtual bool isEof() noexcept override {
		return feof(fp);
	}

	virtual loader::ReadResult read(char *buffer, size_t size) noexcept override {
		if (fread(buffer, size, 1, fp) < 1) {
			return loader::ReadResult::ReadError;
		}
		return loader::ReadResult::Succeeded;
	}

	virtual void dealloc() noexcept override {
		peff::destroyAndRelease<MyReader>(selfAllocator.get(), this, alignof(MyReader));
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

class DumpWriter {
public:
	SLAKE_API virtual ~DumpWriter() {
	}
	[[nodiscard]] virtual bool write(const char *data, size_t len) = 0;

	SLAKE_FORCEINLINE bool write(const std::string_view &s) {
		return write(s.data(), s.size());
	}
};

[[nodiscard]] SLAKE_API bool dumpTypeName(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type);
[[nodiscard]] SLAKE_API bool dumpValue(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value);
[[nodiscard]] SLAKE_API bool dumpIdRefEntries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &idRefIn);
[[nodiscard]] SLAKE_API bool dumpIdRef(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *idRefIn);

#define SLAKE_RETURN_IF_FALSE(e) \
	if (!e) return false

SLAKE_API bool dumpValue(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value) {
	switch (value.valueType) {
		case slake::ValueType::I8: {
			char s[8];
			sprintf(s, "%hd", (int16_t)value.getI8());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I16: {
			char s[16];
			sprintf(s, "%hd", (int16_t)value.getI16());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I32: {
			char s[32];
			sprintf(s, "%d", value.getI32());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I64: {
			char s[48];
			sprintf(s, "%lld", value.getI64());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U8: {
			char s[4];
			sprintf(s, "%hu", (uint16_t)value.getU8());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U16: {
			char s[8];
			sprintf(s, "%hu", (uint16_t)value.getU16());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U32: {
			char s[16];
			sprintf(s, "%u", value.getU32());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U64: {
			char s[32];
			sprintf(s, "%llu", value.getU64());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F32: {
			char s[16];
			sprintf(s, "%f", value.getF32());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F64: {
			char s[32];
			sprintf(s, "%f", value.getF64());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::Bool:
			SLAKE_RETURN_IF_FALSE(writer->write(value.getBool() ? "true" : "false"));
			break;
		case slake::ValueType::Reference: {
			const slake::Reference &er = value.getReference();

			switch (er.kind) {
				case slake::ReferenceKind::ObjectRef: {
					slake::Object *obj = er.asObject;

					if (!obj) {
						SLAKE_RETURN_IF_FALSE(writer->write("null"));
						break;
					}

					switch (obj->getObjectKind()) {
						case slake::ObjectKind::String: {
							SLAKE_RETURN_IF_FALSE(writer->write("\""));

							slake::StringObject *s = (slake::StringObject *)obj;

							const char *data = s->data.data();
							size_t len = s->data.size();
							char c;

							size_t idxCharSinceLastEsc = 0;

							for (size_t i = 0; i < len; ++i) {
								switch ((c = data[i])) {
									case '\n':
									case '\t':
									case '\v':
									case '\f':
									case '\a':
									case '\b':
									case '\r':
									case '"':
									case '\\':
										SLAKE_RETURN_IF_FALSE(writer->write(std::string_view(data + idxCharSinceLastEsc, i - idxCharSinceLastEsc)));

										switch (c) {
											case '\0':
												SLAKE_RETURN_IF_FALSE(writer->write("\\0"));
												break;
											case '\n':
												SLAKE_RETURN_IF_FALSE(writer->write("\\n"));
												break;
											case '\t':
												SLAKE_RETURN_IF_FALSE(writer->write("\\t"));
												break;
											case '\v':
												SLAKE_RETURN_IF_FALSE(writer->write("\\v"));
												break;
											case '\f':
												SLAKE_RETURN_IF_FALSE(writer->write("\\f"));
												break;
											case '\a':
												SLAKE_RETURN_IF_FALSE(writer->write("\\a"));
												break;
											case '\b':
												SLAKE_RETURN_IF_FALSE(writer->write("\\b"));
												break;
											case '\r':
												SLAKE_RETURN_IF_FALSE(writer->write("\\r"));
												break;
											case '"':
												SLAKE_RETURN_IF_FALSE(writer->write("\\\""));
												break;
											case '\\':
												SLAKE_RETURN_IF_FALSE(writer->write("\\\\"));
												break;
										}

										idxCharSinceLastEsc = i + 1;
										break;

									default:

										break;
								}
							}

							if (idxCharSinceLastEsc < len)
								SLAKE_RETURN_IF_FALSE(writer->write(std::string_view(data + idxCharSinceLastEsc, len - idxCharSinceLastEsc)));

							SLAKE_RETURN_IF_FALSE(writer->write("\""));
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj));
							break;
						}
						case slake::ObjectKind::Array: {
							slake::ArrayObject *a = (slake::ArrayObject *)obj;

							SLAKE_RETURN_IF_FALSE(writer->write("{ "));

							for (size_t i = 0; i < a->length; ++i) {
								if (i) {
									SLAKE_RETURN_IF_FALSE(writer->write(", "));
								}

								slake::Reference rer = slake::ArrayElementRef(a, i);
								slake::Value data;

								slake::Runtime::readVar(rer, data);

								SLAKE_RETURN_IF_FALSE(dumpValue(allocator, writer, data));
							}

							SLAKE_RETURN_IF_FALSE(writer->write(" }"));
						}
						default:
							std::terminate();
					}
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::ValueType::RegIndex: {
			char s[32];
			sprintf(s, "%%%u", value.getRegIndex());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::TypeName: {
			SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, value.getTypeName()));
			break;
		}
		default:
			std::terminate();
	}

	return true;
}

SLAKE_API bool dumpIdRefEntries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &idRefIn) {
	for (size_t i = 0; i < idRefIn.size(); ++i) {
		if (i) {
			SLAKE_RETURN_IF_FALSE(writer->write("."));
		}

		auto &curEntry = idRefIn.at(i);

		SLAKE_RETURN_IF_FALSE(writer->write(curEntry.name));

		if (curEntry.genericArgs.size()) {
			SLAKE_RETURN_IF_FALSE(writer->write("<"));
			for (size_t j = 0; j < curEntry.genericArgs.size(); ++j) {
				if (j) {
					SLAKE_RETURN_IF_FALSE(writer->write(","));
				}
				SLAKE_RETURN_IF_FALSE(dumpValue(allocator, writer, curEntry.genericArgs.at(j)));
			}
			SLAKE_RETURN_IF_FALSE(writer->write(">"));
		}
	}

	return true;
}

SLAKE_API bool dumpIdRef(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *idRefIn) {
	SLAKE_RETURN_IF_FALSE(dumpIdRefEntries(allocator, writer, idRefIn->entries));

	if (idRefIn->paramTypes.hasValue()) {
		auto &paramTypes = *idRefIn->paramTypes;

		if (paramTypes.size()) {
			SLAKE_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < paramTypes.size(); ++i) {
				if (i) {
					SLAKE_RETURN_IF_FALSE(writer->write(", "));
				}

				SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, paramTypes.at(i)));
			}

			if (idRefIn->hasVarArgs) {
				SLAKE_RETURN_IF_FALSE(writer->write(", ..."));
			}

			SLAKE_RETURN_IF_FALSE(writer->write(")"));
		} else {
			if (idRefIn->hasVarArgs) {
				SLAKE_RETURN_IF_FALSE(writer->write("(...)"));
			} else {
				SLAKE_RETURN_IF_FALSE(writer->write("()"));
			}
		}
	} else {
		if (idRefIn->hasVarArgs) {
			SLAKE_RETURN_IF_FALSE(writer->write("(...)"));
		}
	}

	if (idRefIn->overridenType) {
		SLAKE_RETURN_IF_FALSE(writer->write(" override "));
		SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, idRefIn->overridenType));
	}

	return true;
}

SLAKE_API bool dumpTypeName(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type) {
	switch (type.typeId) {
		case slake::TypeId::Invalid:
			SLAKE_RETURN_IF_FALSE(writer->write("/* Invalid type */"));
			break;
		case slake::TypeId::Void:
			SLAKE_RETURN_IF_FALSE(writer->write("void"));
			break;
		case slake::TypeId::I8:
			SLAKE_RETURN_IF_FALSE(writer->write("i8"));
			break;
		case slake::TypeId::I16:
			SLAKE_RETURN_IF_FALSE(writer->write("i16"));
			break;
		case slake::TypeId::I32:
			SLAKE_RETURN_IF_FALSE(writer->write("i32"));
			break;
		case slake::TypeId::I64:
			SLAKE_RETURN_IF_FALSE(writer->write("i64"));
			break;
		case slake::TypeId::U8:
			SLAKE_RETURN_IF_FALSE(writer->write("u8"));
			break;
		case slake::TypeId::U16:
			SLAKE_RETURN_IF_FALSE(writer->write("u16"));
			break;
		case slake::TypeId::U32:
			SLAKE_RETURN_IF_FALSE(writer->write("u32"));
			break;
		case slake::TypeId::U64:
			SLAKE_RETURN_IF_FALSE(writer->write("u64"));
			break;
		case slake::TypeId::F32:
			SLAKE_RETURN_IF_FALSE(writer->write("f32"));
			break;
		case slake::TypeId::F64:
			SLAKE_RETURN_IF_FALSE(writer->write("f64"));
			break;
		case slake::TypeId::Bool:
			SLAKE_RETURN_IF_FALSE(writer->write("bool"));
			break;
		case slake::TypeId::String:
			SLAKE_RETURN_IF_FALSE(writer->write("string"));
			break;
		case slake::TypeId::Instance: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					SLAKE_RETURN_IF_FALSE(writer->write("@"));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("@"));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::StructInstance: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Struct: {
					SLAKE_RETURN_IF_FALSE(writer->write("struct "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("struct "));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::ScopedEnum: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Struct: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum base "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum base "));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::TypelessScopedEnum: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Struct: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum "));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::UnionEnum: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Struct: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum union "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum union "));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::UnionEnumItem: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Struct: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum struct "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum struct "));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::GenericArg: {
			auto obj = type.getGenericArgTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			SLAKE_RETURN_IF_FALSE(writer->write("@!"));
			SLAKE_RETURN_IF_FALSE(writer->write(obj->nameObject->data.data(), obj->nameObject->data.size()));
			break;
		}
		case slake::TypeId::Array: {
			auto obj = type.getArrayTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, obj->elementType->typeRef));
			SLAKE_RETURN_IF_FALSE(writer->write("[]"));
			break;
		}
		case slake::TypeId::Ref: {
			auto obj = type.getRefTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, obj->referencedType->typeRef));
			SLAKE_RETURN_IF_FALSE(writer->write("&"));
			break;
		}
		case slake::TypeId::Fn: {
			SLAKE_RETURN_IF_FALSE(writer->write("(fn delegate, not implemented yet)"));
			break;
		}
		case slake::TypeId::Any:
			SLAKE_RETURN_IF_FALSE(writer->write("any"));
			break;
		case slake::TypeId::ParamTypeList: {
			auto obj = type.getParamTypeListTypeDef();

			SLAKE_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < obj->paramTypes.size(); ++i) {
				if (i) {
					SLAKE_RETURN_IF_FALSE(writer->write(", "));
				}

				SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, obj->paramTypes.at(i)->typeRef));
			}

			SLAKE_RETURN_IF_FALSE(writer->write(")"));
			break;
		}
		case slake::TypeId::Tuple: {
			auto obj = type.getTupleTypeDef();

			SLAKE_RETURN_IF_FALSE(writer->write("["));

			for (size_t i = 0; i < obj->elementTypes.size(); ++i) {
				if (i) {
					SLAKE_RETURN_IF_FALSE(writer->write(", "));
				}

				SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, obj->elementTypes.at(i)->typeRef));
			}

			SLAKE_RETURN_IF_FALSE(writer->write("]"));
			break;
		}
		case slake::TypeId::SIMD: {
			auto obj = type.getSIMDTypeDef();

			SLAKE_RETURN_IF_FALSE(writer->write("simd_t<"));

			SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, obj->type->typeRef));

			SLAKE_RETURN_IF_FALSE(writer->write(", "));

			char s[16];
			sprintf(s, "%u", obj->width);
			SLAKE_RETURN_IF_FALSE(writer->write(s));

			SLAKE_RETURN_IF_FALSE(writer->write(">"));
			break;
		}
		case slake::TypeId::Unpacking: {
			auto obj = type.getUnpackingTypeDef();
			SLAKE_RETURN_IF_FALSE(writer->write("@..."));
			SLAKE_RETURN_IF_FALSE(dumpTypeName(allocator, writer, obj->type->typeRef));
			break;
		}
		default:
			std::terminate();
	}

	if (type.isNullable())
		SLAKE_RETURN_IF_FALSE(writer->write("?"));
	return true;
}

bool dumpExceptionInfo(peff::Alloc *allocator, DumpWriter *writer, slake::InternalException *e) {
	SLAKE_RETURN_IF_FALSE(writer->write(e->what()));
	SLAKE_RETURN_IF_FALSE(writer->write("\n"));
	auto writeDetailsHeader = [writer]() -> bool {
		SLAKE_RETURN_IF_FALSE(writer->write("Details:\n"));
		return true;
	};
	switch (e->kind) {
		case ErrorKind::OutOfMemoryError:
			break;
		case ErrorKind::RuntimeExecError: {
			RuntimeExecError *rte = (RuntimeExecError *)e;
			switch (rte->errorCode) {
				case slake::RuntimeExecErrorCode::MismatchedVarType: {
					MismatchedVarTypeError *err = (MismatchedVarTypeError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Mismatched variable type"));
					break;
				}
				case slake::RuntimeExecErrorCode::ReferencedMemberNotFound: {
					ReferencedMemberNotFoundError *err = (ReferencedMemberNotFoundError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Referenced member not found: "));
					SLAKE_RETURN_IF_FALSE(dumpIdRef(allocator, writer, err->idRef.get()));
					break;
				}
				case slake::RuntimeExecErrorCode::UncaughtException: {
					UncaughtExceptionError *err = (UncaughtExceptionError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Uncaught exception: "));
					SLAKE_RETURN_IF_FALSE(dumpValue(allocator, writer, err->exceptionValue));
					break;
				}
				case slake::RuntimeExecErrorCode::FrameBoundaryExceeded: {
					FrameBoundaryExceededError *err = (FrameBoundaryExceededError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Frame boundary exceeded"));
					break;
				}
				case slake::RuntimeExecErrorCode::InvalidArgumentNumber: {
					InvalidArgumentNumberError *err = (InvalidArgumentNumberError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Invalid argument number, "));
					{
						char n[13];
						sprintf(n, "%u", err->nArgs);
						SLAKE_RETURN_IF_FALSE(writer->write(n));
					}
					SLAKE_RETURN_IF_FALSE(writer->write(" arguments does not match"));
					break;
				}
				case slake::RuntimeExecErrorCode::InvalidArrayIndex: {
					InvalidArrayIndexError *err = (InvalidArrayIndexError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Invalid array index, "));
					{
						char n[26];
						sprintf(n, "%zu", err->index);
						SLAKE_RETURN_IF_FALSE(writer->write(n));
					}
					SLAKE_RETURN_IF_FALSE(writer->write(" is out of index range"));
					break;
				}
				case slake::RuntimeExecErrorCode::StackOverflow: {
					StackOverflowError *err = (StackOverflowError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Stack overflowed"));
					break;
				}
				case slake::RuntimeExecErrorCode::MalformedClassStructure: {
					MalformedClassStructureError *err = (MalformedClassStructureError *)rte;
					SLAKE_RETURN_IF_FALSE(writeDetailsHeader());
					SLAKE_RETURN_IF_FALSE(writer->write("Malformed class structure"));
					break;
				}
					// TODO: Implement them all.
			}
		}
	}

	puts("");

	return true;
}

class StdDumpWriter : public DumpWriter {
public:
	FILE *stream;
	StdDumpWriter(FILE *stream) : stream(stream) {
		assert((stream == stdout) || (stream == stdin) || (stream == stderr));
	}

	SLAKE_API virtual ~StdDumpWriter() {
	}

	virtual bool write(const char *src, size_t size) override {
		fwrite(src, size, 1, stream);
		return true;
	}
};

int main(int argc, char **argv) {
	util::setupMemoryLeakDetector();

	StdDumpWriter stderrWriter(stderr);
	{
		std::unique_ptr<Runtime, peff::DeallocableDeleter<Runtime>> rt = std::unique_ptr<Runtime, peff::DeallocableDeleter<Runtime>>(
			Runtime::alloc(
				peff::getDefaultAlloc(),
				peff::getDefaultAlloc(),
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

				closeFpGuard.release();

				if (auto e = loader::loadModule(loaderContext, rt.get(), &reader, mod); e) {
					printf("Error loading main module:\n");
					if (!dumpExceptionInfo(peff::getDefaultAlloc(), &stderrWriter, e.get()))
						abort();
					e.reset();
					return -1;
				}
			}

			{
				HostRefHolder hostRefHolder(rt->getFixedAlloc());

				HostObjectRef<ModuleObject> modObjectHostext;

				modObjectHostext = (ModuleObject *)rt->getRootObject()->getMember("hostext").getObjectRef();
				if ((!modObjectHostext) || (modObjectHostext->getName() != "hostext"))
					std::terminate();

				/*
				if (!modObjectHostext->setName("hostext")) {
					std::terminate();
				}*/
				HostObjectRef<ModuleObject> modObjectExtfns = ModuleObject::alloc(rt.get());
				if (!modObjectExtfns)
					std::terminate();
				if (!modObjectExtfns->setName("extfns")) {
					std::terminate();
				}

				if (!modObjectHostext->addMember(modObjectExtfns.get())) {
					std::terminate();
				}

				modObjectExtfns = (ModuleObject *)modObjectHostext->getMember("extfns").getObjectRef();
				if ((!modObjectExtfns) || (modObjectExtfns->getName() != "extfns"))
					std::terminate();

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

				modObjectExtfns->removeMember("print");
				if (!modObjectExtfns->addMember(fnObject.get()))
					throw std::bad_alloc();

				auto fn = (FnObject *)mod->getMember("main").asObject;
				FnOverloadingObject *overloading;

				peff::DynArray<TypeRef> params(peff::getDefaultAlloc());

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

							if (!dumpExceptionInfo(peff::getDefaultAlloc(), &stderrWriter, e.get()))
								abort();

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
