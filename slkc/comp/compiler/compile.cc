#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileTypeName(
	slake::Runtime *runtime,
	slake::HostRefHolder &hostRefHolder,
	peff::SharedPtr<TypeNameNode> typeName,
	slake::Type &typeOut) {
	switch (typeName->typeNameKind) {
		case TypeNameKind::Void:
			typeOut = slake::Type(slake::TypeId::None);
			break;
		case TypeNameKind::I8:
			typeOut = slake::Type(slake::TypeId::I8);
			break;
		case TypeNameKind::I16:
			typeOut = slake::Type(slake::TypeId::I16);
			break;
		case TypeNameKind::I32:
			typeOut = slake::Type(slake::TypeId::I32);
			break;
		case TypeNameKind::I64:
			typeOut = slake::Type(slake::TypeId::I64);
			break;
		case TypeNameKind::U8:
			typeOut = slake::Type(slake::TypeId::U8);
			break;
		case TypeNameKind::U16:
			typeOut = slake::Type(slake::TypeId::U16);
			break;
		case TypeNameKind::U32:
			typeOut = slake::Type(slake::TypeId::U32);
			break;
		case TypeNameKind::U64:
			typeOut = slake::Type(slake::TypeId::U64);
			break;
		case TypeNameKind::String:
			typeOut = slake::Type(slake::TypeId::String);
			break;
		case TypeNameKind::Bool:
			typeOut = slake::Type(slake::TypeId::Bool);
			break;
		case TypeNameKind::Object:
			typeOut = slake::Type(slake::TypeId::Instance, nullptr);
			break;
		case TypeNameKind::Any:
			typeOut = slake::Type(slake::TypeId::Any);
			break;
		case TypeNameKind::Custom: {
			peff::SharedPtr<CustomTypeNameNode> t = typeName.castTo<CustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document.lock();
			peff::SharedPtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(Compiler::resolveCustomTypeName(doc, t, m));

			if (!m) {
				return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			switch (m->astNodeType) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					IdRefPtr fullName;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(doc->allocator.get(), m, fullName));

					slake::HostObjectRef<slake::IdRefObject> obj;

					if (!(obj = slake::IdRefObject::alloc(runtime))) {
						return genOutOfMemoryCompError();
					}

					if (!(hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(runtime, hostRefHolder, fullName->entries.data(), fullName->entries.size(), obj->entries, nullptr, 0, false));

					typeOut = slake::Type(slake::TypeId::Instance, obj.get());
					break;
				}
				case AstNodeType::GenericParam: {
					slake::HostObjectRef<slake::StringObject> obj;

					peff::String s(doc->allocator.get());

					if (!s.build(m->name)) {
						return genOutOfMemoryCompError();
					}

					if (!(obj = slake::StringObject::alloc(runtime, std::move(s)))) {
						return genOutOfMemoryCompError();
					}

					if (!(hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeOut = slake::Type(obj.get(), /* Placeholder!!! */ nullptr);
					break;
				}
				default:
					return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			break;
		}
		case TypeNameKind::Array: {
			peff::SharedPtr<ArrayTypeNameNode> t = typeName.castTo<ArrayTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document.lock();

			slake::Type st;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, t->elementType, st));

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(runtime, st))) {
				return genOutOfMemoryCompError();
			}

			if (!(hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Array, obj.get());
			break;
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode> t = typeName.castTo<RefTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document.lock();

			slake::Type st;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, t->referencedType, st));

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(runtime, st))) {
				return genOutOfMemoryCompError();
			}

			if (!(hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Ref, obj.get());
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileIdRef(
	slake::Runtime *runtime,
	slake::HostRefHolder &hostRefHolder,
	const IdRefEntry *entries,
	size_t nEntries,
	peff::DynArray<slake::IdRefEntry> &entriesOut,
	peff::SharedPtr<TypeNameNode> *paramTypes,
	size_t nParams,
	bool hasVarArgs) {
	if (!entriesOut.resizeUninitialized(nEntries)) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < entriesOut.size(); ++i) {
		peff::constructAt<slake::IdRefEntry>(&entriesOut.at(i), slake::IdRefEntry(entriesOut.allocator()));
	}

	for (size_t i = 0; i < nEntries; ++i) {
		const IdRefEntry &ce = entries[i];
		slake::IdRefEntry &e = entriesOut.at(i);

		if (!e.name.build(ce.name)) {
			return genOutOfMemoryCompError();
		}

		if (!e.genericArgs.resize(ce.genericArgs.size())) {
			return genOutOfMemoryCompError();
		}

		for (size_t i = 0; i < ce.genericArgs.size(); ++i) {
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, ce.genericArgs.at(i), e.genericArgs.at(i)));
		}
	}

	slake::IdRefEntry &e = entriesOut.back();

	if (!e.paramTypes.resize(nParams)) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < nParams; ++i) {
		SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, paramTypes[i], e.genericArgs.at(i)));
	}

	e.hasVarArg = hasVarArgs;

	return {};
}
