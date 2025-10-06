#include "../compiler.h"

using namespace slkc;

SLKC_API Writer::~Writer() {
}

SLKC_API std::optional<CompilationError> slkc::dumpGenericParam(
	peff::Alloc *allocator,
	Writer *writer,
	const slake::GenericParam &genericParam) {
	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(genericParam.name.size()));
	SLKC_RETURN_IF_COMP_ERROR(writer->write(genericParam.name.data(), genericParam.name.size()));

	bool hasBaseType = genericParam.baseType != slake::TypeId::Any;
	SLKC_RETURN_IF_COMP_ERROR(writer->writeBool(hasBaseType));
	if (hasBaseType)
		SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, genericParam.baseType));

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(genericParam.interfaces.size()));
	for (auto &k : genericParam.interfaces) {
		SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, k));
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpIdRefEntries(
	peff::Alloc *allocator,
	Writer *writer,
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
	peff::Alloc *allocator,
	Writer *writer,
	slake::IdRefObject *ref) {
	SLKC_RETURN_IF_COMP_ERROR(dumpIdRefEntries(allocator, writer, ref->entries));

	if (ref->entries.at(0).name == "")
		puts("");
	if (!ref->paramTypes.hasValue()) {
		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(UINT32_MAX));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)ref->paramTypes->size()));
		for (auto &i : *ref->paramTypes) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, i));
		}
	}
	SLKC_RETURN_IF_COMP_ERROR(writer->writeBool(ref->hasVarArgs));
	return {};
}

[[nodiscard]] SLKC_API std::optional<CompilationError> slkc::dumpValue(
	peff::Alloc *allocator,
	Writer *writer,
	const slake::Value &value) {
	switch (value.valueType) {
		case slake::ValueType::I8:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I8));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeI8(value.getI8()));
			break;
		case slake::ValueType::I16:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I16));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeI16(value.getI16()));
			break;
		case slake::ValueType::I32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I32));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeI32(value.getI32()));
			break;
		case slake::ValueType::I64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::I64));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeI64(value.getI64()));
			break;
		case slake::ValueType::U8:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U8));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8(value.getU8()));
			break;
		case slake::ValueType::U16:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U16));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU16(value.getU16()));
			break;
		case slake::ValueType::U32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U32));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(value.getU32()));
			break;
		case slake::ValueType::U64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::U64));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU64(value.getU64()));
			break;
		case slake::ValueType::F32:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::F32));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeF32(value.getF32()));
			break;
		case slake::ValueType::F64:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::F64));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeF64(value.getF64()));
			break;
		case slake::ValueType::Bool:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::Bool));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeBool(value.getI8()));
			break;
		case slake::ValueType::TypeName:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::TypeName));
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, value.getTypeName()));
			break;
		case slake::ValueType::RegRef:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::Reg));
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(value.getRegIndex()));
			break;
		case slake::ValueType::EntityRef: {
			const slake::EntityRef &er = value.getEntityRef();

			switch (er.kind) {
				case slake::EntityRefKind::ObjectRef: {
					if (!er.asObject) {
						SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::None));
						break;
					}
					switch (er.asObject->getObjectKind()) {
						case slake::ObjectKind::String: {
							slake::StringObject *s = (slake::StringObject *)er.asObject;
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::String));
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(s->data.size()));
							if (s->data.size()) {
								SLKC_RETURN_IF_COMP_ERROR(writer->write(s->data.data(), s->data.size()));
							}
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::IdRef));
							SLKC_RETURN_IF_COMP_ERROR(dumpIdRef(allocator, writer, (slake::IdRefObject *)er.asObject));
							break;
						}
						case slake::ObjectKind::Array: {
							slake::ArrayObject *a = (slake::ArrayObject *)er.asObject;
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::ValueType::Array));
							SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, a->elementType));
							SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(a->length));
							if (a->length) {
								SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)a->data, a->elementSize * a->length));
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
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpTypeName(
	peff::Alloc *allocator,
	Writer *writer,
	const slake::TypeRef &type) {
	switch (type.typeId) {
		case slake::TypeId::Void:
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Void));
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
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, type.getArrayTypeDef()->elementType->typeRef));
			break;
		case slake::TypeId::Instance: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Object));
			slake::Object *dest = type.getCustomTypeDef()->typeObject;
			switch (dest->getObjectKind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dumpIdRef(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associatedRuntime->getFullRef(allocator, (slake::MemberObject *)dest, entries)) {
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
		case slake::TypeId::StructInstance: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Struct));
			slake::Object *dest = type.getCustomTypeDef()->typeObject;
			switch (dest->getObjectKind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dumpIdRef(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::Struct: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associatedRuntime->getFullRef(allocator, (slake::MemberObject *)dest, entries)) {
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
			slake::StringObject *dest = type.getGenericArgTypeDef()->nameObject;
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(dest->data.size()));
			SLKC_RETURN_IF_COMP_ERROR(writer->write(dest->data.data(), dest->data.size()));
			break;
		}
		case slake::TypeId::Ref: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Ref));
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, type.getRefTypeDef()->referencedType->typeRef));
			break;
		}
		case slake::TypeId::ParamTypeList: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::ParamTypeList));

			slake::ParamTypeListTypeDefObject *typeDef = type.getParamTypeListTypeDef();

			SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)typeDef->paramTypes.size()));

			for (size_t i = 0; i < typeDef->paramTypes.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, typeDef->paramTypes.at(i)->typeRef));
			}

			SLKC_RETURN_IF_COMP_ERROR(writer->writeBool(typeDef->hasVarArg));

			break;
		}
		case slake::TypeId::Unpacking: {
			SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)slake::slxfmt::TypeId::Unpacking));
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, type.getUnpackingTypeDef()->type->typeRef));
			break;
		}
		default:
			std::terminate();
	}

	uint8_t typeModifier = 0;

	if (type.typeModifier & slake::TYPE_FINAL) {
		typeModifier |= slake::slxfmt::TYPE_FINAL;
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU8(typeModifier));

	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpModuleMembers(
	peff::Alloc *allocator,
	Writer *writer,
	slake::ModuleObject *mod) {
	peff::DynArray<slake::ClassObject *> collectedClasses(allocator);
	peff::DynArray<slake::InterfaceObject *> collectedInterfaces(allocator);
	peff::DynArray<slake::FnObject *> collectedFns(allocator);
	peff::DynArray<slake::StructObject *> collectedStructs(allocator);

	for (auto [k, v] : mod->members) {
		switch (v->getObjectKind()) {
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
			case slake::ObjectKind::Struct: {
				if (!collectedStructs.pushBack((slake::StructObject *)v)) {
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
		slake::slxfmt::ClassTypeDesc desc = {};

		if (i->accessModifier & slake::ACCESS_PUB) {
			desc.flags |= slake::slxfmt::CTD_PUB;
		}
		if (i->accessModifier & slake::ACCESS_FINAL) {
			desc.flags |= slake::slxfmt::CTD_FINAL;
		}
		if (i->baseType) {
			desc.flags |= slake::slxfmt::CTD_DERIVED;
		}
		desc.nGenericParams = i->genericParams.size();
		desc.lenName = i->name.size();
		desc.nImpls = i->implTypes.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

		for (auto &j : i->genericParams) {
			SLKC_RETURN_IF_COMP_ERROR(dumpGenericParam(allocator, writer, j));
		}

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
		slake::slxfmt::InterfaceTypeDesc desc = {};

		if (i->accessModifier & slake::ACCESS_PUB) {
			desc.flags |= slake::slxfmt::ITD_PUB;
		}
		desc.nGenericParams = i->genericParams.size();
		desc.lenName = i->name.size();
		desc.nParents = i->implTypes.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

		for (auto &j : i->genericParams) {
			SLKC_RETURN_IF_COMP_ERROR(dumpGenericParam(allocator, writer, j));
		}

		for (auto &j : i->implTypes) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dumpModuleMembers(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(collectedStructs.size()));
	for (auto i : collectedStructs) {
		slake::slxfmt::StructTypeDesc desc = {};

		if (i->accessModifier & slake::ACCESS_PUB) {
			desc.flags |= slake::slxfmt::STD_PUB;
		}
		desc.nGenericParams = i->genericParams.size();
		desc.lenName = i->name.size();
		desc.nImpls = i->implTypes.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

		for (auto &j : i->genericParams) {
			SLKC_RETURN_IF_COMP_ERROR(dumpGenericParam(allocator, writer, j));
		}

		for (auto &j : i->implTypes) {
			SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dumpModuleMembers(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(collectedFns.size()));
	for (auto i : collectedFns) {
		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(i->name.size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->name.data(), i->name.size()));

		SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(i->overloadings.size()));
		for (auto j : i->overloadings) {
			if (j.second->overloadingKind != slake::FnOverloadingKind::Regular) {
				// stub
				continue;
			}

			slake::RegularFnOverloadingObject *ol = (slake::RegularFnOverloadingObject *)j.second;

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
				fnd.flags |= slake::slxfmt::FND_VARG;
			}
			if (ol->overloadingFlags & slake::OL_GENERATOR) {
				fnd.flags |= slake::slxfmt::FND_GENERATOR;
			}
			if (ol->overloadingFlags & slake::OL_VIRTUAL) {
				fnd.flags |= slake::slxfmt::FND_VIRTUAL;
			}
			fnd.nParams = ol->paramTypes.size();
			fnd.nGenericParams = ol->genericParams.size();
			fnd.nRegisters = ol->nRegisters;
			fnd.lenBody = ol->instructions.size();

			SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&fnd, sizeof(fnd)));

			for (auto &k : j.second->genericParams) {
				SLKC_RETURN_IF_COMP_ERROR(dumpGenericParam(allocator, writer, k));
			}

			for (auto &k : ol->paramTypes) {
				SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, k));
			}

			for (auto &k : ol->instructions) {
				SLKC_RETURN_IF_COMP_ERROR(writer->writeU8((uint8_t)k.opcode));
				SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)k.output));
				SLKC_RETURN_IF_COMP_ERROR(writer->writeU32((uint32_t)k.nOperands));
				for (size_t l = 0; l < k.nOperands; ++l) {
					SLKC_RETURN_IF_COMP_ERROR(dumpValue(allocator, writer, k.operands[l]));
				}
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->writeU32(mod->fieldRecords.size()));
	for (size_t i = 0; i < mod->fieldRecords.size(); ++i) {
		auto &curRecord = mod->fieldRecords.at(i);

		slake::slxfmt::VarDesc vad = {};

		if (curRecord.accessModifier & slake::ACCESS_PUB) {
			vad.flags |= slake::slxfmt::VAD_PUB;
		}

		if (curRecord.accessModifier & slake::ACCESS_FINAL) {
			vad.flags |= slake::slxfmt::VAD_FINAL;
		}

		if (curRecord.accessModifier & slake::ACCESS_STATIC) {
			vad.flags |= slake::slxfmt::VAD_STATIC;
		}

		if (curRecord.accessModifier & slake::ACCESS_NATIVE) {
			vad.flags |= slake::slxfmt::VAD_NATIVE;
		}

		vad.lenName = curRecord.name.size();
		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&vad, sizeof(vad)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(curRecord.name.data(), curRecord.name.size()));

		SLKC_RETURN_IF_COMP_ERROR(dumpTypeName(allocator, writer, curRecord.type));
		SLKC_RETURN_IF_COMP_ERROR(dumpValue(allocator, writer, mod->associatedRuntime->readVarUnsafe(slake::EntityRef::makeStaticFieldRef(mod, i))));
	}
	return {};
}

SLKC_API std::optional<CompilationError> slkc::dumpModule(
	peff::Alloc *allocator,
	Writer *writer,
	slake::ModuleObject *mod) {
	slake::slxfmt::ImgHeader ih = {};

	memcpy(ih.magic, slake::slxfmt::IMH_MAGIC, sizeof(ih.magic));

	ih.fmtVer = 0x02;
	ih.nImports = mod->unnamedImports.size();

	SLKC_RETURN_IF_COMP_ERROR(writer->write((const char *)&ih, sizeof(ih)));

	peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

	if (!mod->associatedRuntime->getFullRef(allocator, mod, moduleFullName))
		return genOutOfMemoryCompError();

	SLKC_RETURN_IF_COMP_ERROR(dumpIdRefEntries(allocator, writer, moduleFullName));

	for (auto i : mod->unnamedImports) {
		SLKC_RETURN_IF_COMP_ERROR(dumpIdRef(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(dumpModuleMembers(allocator, writer, mod));

	return {};
}
