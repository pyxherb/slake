#include "../compiler.h"

using namespace slkc;

SLKC_API Writer::~Writer() {
}

SLKC_API std::optional<CompilationError> slkc::dumpGenericParams(
	peff::Alloc* allocator,
	Writer* writer,
	const slake::GenericParamList& genericParams) {
	for (auto &j : genericParams) {
		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(j.name.size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->write(j.name.data(), j.name.size()));

		SLKC_RETURN_IF_COMP_ERROR(writer->writeBool(j.baseType));
		SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, j.baseType));

		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(j.interfaces.size()));
		for (auto &k : j.interfaces) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, k));
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpIdRefEntries(
	peff::Alloc* allocator,
	Writer* writer,
	const peff::DynArray<slake::IdRefEntry> &entries) {
	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)entries.size()));
	for (auto &i : entries) {
		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(i.name.size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->write(i.name.data(), i.name.size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->writeU8(i.genericArgs.size()));
		for (auto &j : i.genericArgs) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, j));
		}
	}
	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpIdRef(
	peff::Alloc* allocator,
	Writer* writer,
	slake::IdRefObject* ref) {
	SLKC_RETURN_IF_COMP_ERROR(dumpIdRefEntries(allocator, writer, ref->entries));

	SLKC_RETURN_IF_COMP_ERROR(writer->writeI32((int32_t)ref->paramTypes.size()));
	for (auto &i : ref->paramTypes) {
		SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, i));
	}
	SLKC_RETURN_IF_COMP_ERROR(writer->writeBool(ref->hasVarArgs));
	return {};
}

[[nodiscard]] SLKC_API std::optional<CompilationError> dumpValue(
	peff::Alloc* allocator,
	Writer* writer,
	const slake::Value& value) {
	switch (value.valueType) {
		case slake::ValueType::I8:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I8));
			break;
		case slake::ValueType::I16:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I16));
			break;
		case slake::ValueType::I32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I32));
			break;
		case slake::ValueType::I64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I64));
			break;
		case slake::ValueType::U8:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U8));
			break;
		case slake::ValueType::U16:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U16));
			break;
		case slake::ValueType::U32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U32));
			break;
		case slake::ValueType::U64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U64));
			break;
		case slake::ValueType::F32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::F32));
			break;
		case slake::ValueType::F64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::F64));
			break;
		case slake::ValueType::Bool:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::Bool));
			break;
		case slake::ValueType::TypeName:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::TypeName));
			break;
		case slake::ValueType::RegRef:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::Reg));
			break;
		case slake::ValueType::EntityRef: {
			const slake::EntityRef &er = value.getEntityRef();

			switch (er.kind) {
				case slake::ObjectRefKind::ObjectRef: {
					switch (er.asObject.instanceObject->getKind()) {
						case slake::ObjectKind::String: {
							slake::StringObject *s = (slake::StringObject*)er.asObject.instanceObject;
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::String));
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(s->data.size()));
							SLKC_RETURN_IF_COMP_ERROR(writer->write(s->data.data(), s->data.size()));
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::IdRef));
							SLKC_RETURN_IF_COMP_ERROR(dumpIdRef(allocator, writer, (slake::IdRefObject *)er.asObject.instanceObject));
							break;
						}
						case slake::ObjectKind::Array: {
							slake::ArrayObject *a = (slake::ArrayObject *)er.asObject.instanceObject;
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::Array));
							SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, a->elementType));
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(a->length));
							SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)a->data, a->elementSize * a->length));
							break;
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
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpTypeName(
	peff::Alloc* allocator,
	Writer* writer,
	const slake::Type& type) {
	switch (type.typeId) {
		case slake::TypeId::None:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::None));
			break;
		case slake::TypeId::Any:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Any));
			break;
		case slake::TypeId::I8:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::I8));
			break;
		case slake::TypeId::I16:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::I16));
			break;
		case slake::TypeId::I32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::I32));
			break;
		case slake::TypeId::I64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::I64));
			break;
		case slake::TypeId::U8:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::U8));
			break;
		case slake::TypeId::U16:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::U16));
			break;
		case slake::TypeId::U32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::U32));
			break;
		case slake::TypeId::U64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::U64));
			break;
		case slake::TypeId::F32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::F32));
			break;
		case slake::TypeId::F64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::F64));
			break;
		case slake::TypeId::String:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::String));
			break;
		case slake::TypeId::Bool:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Bool));
			break;
		case slake::TypeId::Array:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Array));
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, type.getArrayExData()));
			break;
		case slake::TypeId::Instance: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Object));
			slake::Object *dest = type.getCustomTypeExData();
			switch (dest->getKind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dumpIdRef(allocator, writer, (slake::IdRefObject*)dest));
					break;
				}
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associatedRuntime->getFullRef(allocator, (slake::MemberObject*)dest, entries)) {
						return genOutOfMemoryCompError();
					}
					SLKC_RETURN_IF_COMP_ERROR(dumpIdRefEntries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::GenericArg: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::GenericArg));
			slake::StringObject *dest = type.exData.genericArg.nameObject;
			SLKC_RETURN_IF_COMP_ERROR(writer->write(dest->data.data(), dest->data.size()));
			break;
		}
		case slake::TypeId::Ref: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Ref));
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, type.getRefExData()));
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpModuleMembers(
	peff::Alloc *allocator,
	Writer *writer,
	slake::ModuleObject *mod) {
	peff::DynArray<slake::ClassObject *> collectedClasses(allocator);
	peff::DynArray<slake::InterfaceObject *> collectedInterfaces(allocator);
	peff::DynArray<slake::FnObject *> collectedFns(allocator);

	for (auto [k, v] : mod->members) {
		switch (v->getKind()) {
			case slake::ObjectKind::Class: {
				if (!collectedClasses.pushBack((slake::ClassObject *)v)) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case slake::ObjectKind::Interface: {
				if (!collectedInterfaces.pushBack((slake::InterfaceObject *)v)) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case slake::ObjectKind::Fn: {
				if (!collectedFns.pushBack((slake::FnObject *)v)) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			default:
				break;
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(collectedClasses.size()));
	for (auto i : collectedClasses) {
		slake::slxfmt::ClassTypeDesc ctd = {};

		if (i->accessModifier & slake::ACCESS_PUB) {
			ctd.flags |= slake::slxfmt::CTD_PUB;
		}
		if (i->accessModifier & slake::ACCESS_FINAL) {
			ctd.flags |= slake::slxfmt::CTD_FINAL;
		}
		if (i->baseType) {
			ctd.flags |= slake::slxfmt::CTD_DERIVED;
		}
		ctd.nGenericParams = i->genericParams.size();
		ctd.lenName = i->name.size();
		ctd.nImpls = i->implTypes.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&ctd, sizeof(ctd)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

		SLKC_RETURN_IF_COMP_ERROR(dumpGenericParams(allocator, writer, i->genericParams));

		if (i->baseType) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, i->baseType));
		}
		for (auto &j : i->implTypes) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dumpModuleMembers(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(collectedInterfaces.size()));
	for (auto i : collectedInterfaces) {
		slake::slxfmt::InterfaceTypeDesc itd = {};

		if (i->accessModifier & slake::ACCESS_PUB) {
			itd.flags |= slake::slxfmt::ITD_PUB;
		}
		itd.nGenericParams = i->genericParams.size();
		itd.lenName = i->name.size();
		itd.nParents = i->implTypes.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&itd, sizeof(itd)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

		SLKC_RETURN_IF_COMP_ERROR(dumpGenericParams(allocator, writer, i->genericParams));

		for (auto &j : i->implTypes) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dumpModuleMembers(allocator, writer, i));
	}

	for (auto i : collectedFns) {
		for (auto j : i->overloadings) {
			if (j->overloadingKind != slake::FnOverloadingKind::Regular) {
				continue;
			}

			slake::RegularFnOverloadingObject *ol = (slake::RegularFnOverloadingObject *)j;

			slake::slxfmt::FnDesc fnd = {};

			if (ol->access & slake::ACCESS_PUB) {
				fnd.flags |= slake::slxfmt::FND_PUB;
			}
			if (ol->access & slake::ACCESS_FINAL) {
				fnd.flags |= slake::slxfmt::FND_FINAL;
			}
			if (ol->access & slake::ACCESS_STATIC) {
				fnd.flags |= slake::slxfmt::FND_STATIC;
			}
			if (ol->access & slake::ACCESS_NATIVE) {
				fnd.flags |= slake::slxfmt::FND_NATIVE;
			}
			if (ol->overloadingFlags & slake::OL_VARG) {
				fnd.flags |= slake::slxfmt::FND_STATIC;
			}
			if (ol->overloadingFlags & slake::OL_GENERATOR) {
				fnd.flags |= slake::slxfmt::FND_GENERATOR;
			}
			if (ol->overloadingFlags & slake::OL_VIRTUAL) {
				fnd.flags |= slake::slxfmt::FND_VIRTUAL;
			}
			fnd.nParams = ol->paramTypes.size();
			fnd.nGenericParams = ol->genericParams.size();
			fnd.lenName = i->name.size();
			fnd.nRegisters = ol->nRegisters;
			fnd.lenBody = ol->instructions.size();

			SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&fnd, sizeof(fnd)));

			SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

			SLKC_RETURN_IF_COMP_ERROR(dumpGenericParams(allocator, writer, j->genericParams));

			for (auto &k : ol->paramTypes) {
				SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, k));
			}

			for (auto &k : ol->instructions) {
				SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)k.opcode));
				SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)k.output));
				SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)k.nOperands));
			}
		}
	}
	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpModule(
	peff::Alloc* allocator,
	Writer* writer,
	slake::ModuleObject *mod) {
	slake::slxfmt::ImgHeader ih = {};

	memcpy(ih.magic, slake::slxfmt::IMH_MAGIC, sizeof(ih.magic));

	ih.fmtVer = 0x02;
	ih.nImports = mod->imports.size() + mod->unnamedImports.size();

	SLKC_RETURN_IF_COMP_ERROR(writer->write((const char *)&ih, sizeof(ih)));

	SLKC_RETURN_IF_COMP_ERROR(dumpModuleMembers(allocator, writer, mod));

	return {};
}
