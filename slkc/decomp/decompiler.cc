#include "decompiler.h"
#include <slake/opti/cfg.h>

using namespace slkc;

#define SLKC_RETURN_IF_FALSE(e) \
	if (!e) return false

SLKC_API DumpWriter::~DumpWriter() {
}

#define MNEMONIC_NAME_CASE(name) \
	case slake::Opcode::name:    \
		return #name;

SLKC_API const char *slkc::getMnemonicName(slake::Opcode opcode) {
	switch (opcode) {
		MNEMONIC_NAME_CASE(INVALID)
		MNEMONIC_NAME_CASE(LOAD)
		MNEMONIC_NAME_CASE(RLOAD)
		MNEMONIC_NAME_CASE(STORE)
		MNEMONIC_NAME_CASE(MOV)
		MNEMONIC_NAME_CASE(LARG)
		MNEMONIC_NAME_CASE(LAPARG)
		MNEMONIC_NAME_CASE(LVAR)
		MNEMONIC_NAME_CASE(ALLOCA)
		MNEMONIC_NAME_CASE(LVALUE)
		MNEMONIC_NAME_CASE(ENTER)
		MNEMONIC_NAME_CASE(LEAVE)
		MNEMONIC_NAME_CASE(ADD)
		MNEMONIC_NAME_CASE(SUB)
		MNEMONIC_NAME_CASE(MUL)
		MNEMONIC_NAME_CASE(DIV)
		MNEMONIC_NAME_CASE(MOD)
		MNEMONIC_NAME_CASE(AND)
		MNEMONIC_NAME_CASE(OR)
		MNEMONIC_NAME_CASE(XOR)
		MNEMONIC_NAME_CASE(LAND)
		MNEMONIC_NAME_CASE(LOR)
		MNEMONIC_NAME_CASE(EQ)
		MNEMONIC_NAME_CASE(NEQ)
		MNEMONIC_NAME_CASE(LT)
		MNEMONIC_NAME_CASE(GT)
		MNEMONIC_NAME_CASE(LTEQ)
		MNEMONIC_NAME_CASE(GTEQ)
		MNEMONIC_NAME_CASE(LSH)
		MNEMONIC_NAME_CASE(RSH)
		MNEMONIC_NAME_CASE(CMP)
		MNEMONIC_NAME_CASE(NOT)
		MNEMONIC_NAME_CASE(LNOT)
		MNEMONIC_NAME_CASE(NEG)
		MNEMONIC_NAME_CASE(AT)
		MNEMONIC_NAME_CASE(JMP)
		MNEMONIC_NAME_CASE(BR)
		MNEMONIC_NAME_CASE(PUSHARG)
		MNEMONIC_NAME_CASE(PUSHAP)
		MNEMONIC_NAME_CASE(CALL)
		MNEMONIC_NAME_CASE(MCALL)
		MNEMONIC_NAME_CASE(CTORCALL)
		MNEMONIC_NAME_CASE(RET)
		MNEMONIC_NAME_CASE(COCALL)
		MNEMONIC_NAME_CASE(COMCALL)
		MNEMONIC_NAME_CASE(YIELD)
		MNEMONIC_NAME_CASE(RESUME)
		MNEMONIC_NAME_CASE(CODONE)
		MNEMONIC_NAME_CASE(LTHIS)
		MNEMONIC_NAME_CASE(NEW)
		MNEMONIC_NAME_CASE(ARRNEW)
		MNEMONIC_NAME_CASE(THROW)
		MNEMONIC_NAME_CASE(PUSHEH)
		MNEMONIC_NAME_CASE(LEXCEPT)
		MNEMONIC_NAME_CASE(CAST)
		MNEMONIC_NAME_CASE(APTOTUPLE)
		MNEMONIC_NAME_CASE(TYPEOF)
		MNEMONIC_NAME_CASE(CONSTSW)
		MNEMONIC_NAME_CASE(PHI)
		default:
			return nullptr;
	}
	std::terminate();
}

SLKC_API Decompiler::Decompiler() {
}

SLKC_API Decompiler::~Decompiler() {
}

SLKC_API bool Decompiler::decompileGenericParam(peff::Alloc *allocator, DumpWriter *writer, const slake::GenericParam &genericParam) {
	SLKC_RETURN_IF_FALSE(writer->write(genericParam.name));
	if (genericParam.inputType != slake::TypeId::Invalid) {
		SLKC_RETURN_IF_FALSE(writer->write(" as "));
		SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, genericParam.inputType));
		if ((genericParam.baseType != slake::TypeId::Invalid) || (genericParam.interfaces.size()))
			SLKC_RETURN_IF_FALSE(writer->write("/* With extraneous constraints */"));
	} else {
		if (genericParam.baseType != slake::TypeId::Invalid) {
			SLKC_RETURN_IF_FALSE(writer->write("("));
			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, genericParam.baseType));
			SLKC_RETURN_IF_FALSE(writer->write(")"));
		}
		if (genericParam.interfaces.size()) {
			SLKC_RETURN_IF_FALSE(writer->write(": "));
			for (size_t i = 0; i < genericParam.interfaces.size(); ++i) {
				if (i)
					SLKC_RETURN_IF_FALSE(writer->write(" + "));
				SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, genericParam.interfaces.at(i)));
			}
		}
	}

	return true;
}

SLKC_API bool Decompiler::decompileTypeName(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type) {
	switch (type.typeId) {
		case slake::TypeId::Void:
			SLKC_RETURN_IF_FALSE(writer->write("void"));
			break;
		case slake::TypeId::I8:
			SLKC_RETURN_IF_FALSE(writer->write("i8"));
			break;
		case slake::TypeId::I16:
			SLKC_RETURN_IF_FALSE(writer->write("i16"));
			break;
		case slake::TypeId::I32:
			SLKC_RETURN_IF_FALSE(writer->write("i32"));
			break;
		case slake::TypeId::I64:
			SLKC_RETURN_IF_FALSE(writer->write("i64"));
			break;
		case slake::TypeId::U8:
			SLKC_RETURN_IF_FALSE(writer->write("u8"));
			break;
		case slake::TypeId::U16:
			SLKC_RETURN_IF_FALSE(writer->write("u16"));
			break;
		case slake::TypeId::U32:
			SLKC_RETURN_IF_FALSE(writer->write("u32"));
			break;
		case slake::TypeId::U64:
			SLKC_RETURN_IF_FALSE(writer->write("u64"));
			break;
		case slake::TypeId::F32:
			SLKC_RETURN_IF_FALSE(writer->write("f32"));
			break;
		case slake::TypeId::F64:
			SLKC_RETURN_IF_FALSE(writer->write("f64"));
			break;
		case slake::TypeId::Bool:
			SLKC_RETURN_IF_FALSE(writer->write("bool"));
			break;
		case slake::TypeId::String:
			SLKC_RETURN_IF_FALSE(writer->write("string"));
			break;
		case slake::TypeId::Instance: {
			auto obj = type.getCustomTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->typeObject->getObjectKind()) {
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					SLKC_RETURN_IF_FALSE(writer->write("@"));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("@"));
					SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
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
					SLKC_RETURN_IF_FALSE(writer->write("struct "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("struct "));
					SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
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
					SLKC_RETURN_IF_FALSE(writer->write("enum "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum "));
					SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
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
					SLKC_RETURN_IF_FALSE(writer->write("enum union "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum union "));
					SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
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
					SLKC_RETURN_IF_FALSE(writer->write("enum struct "));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj->typeObject, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum struct "));
					SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj->typeObject));
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

			SLKC_RETURN_IF_FALSE(writer->write("@!"));
			SLKC_RETURN_IF_FALSE(writer->write(obj->nameObject->data.data(), obj->nameObject->data.size()));
			break;
		}
		case slake::TypeId::Array: {
			auto obj = type.getArrayTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->elementType->typeRef));
			SLKC_RETURN_IF_FALSE(writer->write("[]"));
			break;
		}
		case slake::TypeId::Ref: {
			auto obj = type.getRefTypeDef();

			slake::Runtime *runtime = obj->associatedRuntime;

			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->referencedType->typeRef));
			SLKC_RETURN_IF_FALSE(writer->write("&"));
			break;
		}
		case slake::TypeId::Fn: {
			SLKC_RETURN_IF_FALSE(writer->write("(fn delegate, not implemented yet)"));
			break;
		}
		case slake::TypeId::Any:
			SLKC_RETURN_IF_FALSE(writer->write("any"));
			break;
		case slake::TypeId::ParamTypeList: {
			auto obj = type.getParamTypeListTypeDef();

			SLKC_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < obj->paramTypes.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->paramTypes.at(i)->typeRef));
			}

			SLKC_RETURN_IF_FALSE(writer->write(")"));
			break;
		}
		case slake::TypeId::Tuple: {
			auto obj = type.getTupleTypeDef();

			SLKC_RETURN_IF_FALSE(writer->write("["));

			for (size_t i = 0; i < obj->elementTypes.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->elementTypes.at(i)->typeRef));
			}

			SLKC_RETURN_IF_FALSE(writer->write("]"));
			break;
		}
		case slake::TypeId::SIMD: {
			auto obj = type.getSIMDTypeDef();

			SLKC_RETURN_IF_FALSE(writer->write("simd_t<"));

			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->type->typeRef));

			SLKC_RETURN_IF_FALSE(writer->write(", "));

			char s[16];
			sprintf(s, "%u", obj->width);
			SLKC_RETURN_IF_FALSE(writer->write(s));

			SLKC_RETURN_IF_FALSE(writer->write(">"));
			break;
		}
		case slake::TypeId::Unpacking: {
			auto obj = type.getUnpackingTypeDef();
			SLKC_RETURN_IF_FALSE(writer->write("@..."));
			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->type->typeRef));
			break;
		}
		default:
			std::terminate();
	}

	return true;
}

SLKC_API bool Decompiler::decompileValue(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value) {
	switch (value.valueType) {
		case slake::ValueType::I8: {
			char s[8];
			sprintf(s, "%hd", (int16_t)value.getI8());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I16: {
			char s[16];
			sprintf(s, "%hd", (int16_t)value.getI16());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I32: {
			char s[32];
			sprintf(s, "%d", value.getI32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I64: {
			char s[48];
			sprintf(s, "%lld", value.getI64());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U8: {
			char s[4];
			sprintf(s, "%hu", (uint16_t)value.getU8());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U16: {
			char s[8];
			sprintf(s, "%hu", (uint16_t)value.getU16());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U32: {
			char s[16];
			sprintf(s, "%u", value.getU32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U64: {
			char s[32];
			sprintf(s, "%llu", value.getU64());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F32: {
			char s[16];
			sprintf(s, "%f", value.getF32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F64: {
			char s[32];
			sprintf(s, "%f", value.getF64());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::Bool:
			SLKC_RETURN_IF_FALSE(writer->write(value.getBool() ? "true" : "false"));
			break;
		case slake::ValueType::Reference: {
			const slake::Reference &er = value.getReference();

			switch (er.kind) {
				case slake::ReferenceKind::ObjectRef: {
					slake::Object *obj = er.asObject;

					if (!obj) {
						SLKC_RETURN_IF_FALSE(writer->write("null"));
						break;
					}

					switch (obj->getObjectKind()) {
						case slake::ObjectKind::String: {
							SLKC_RETURN_IF_FALSE(writer->write("\""));

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
										SLKC_RETURN_IF_FALSE(writer->write(std::string_view(data + idxCharSinceLastEsc, i - idxCharSinceLastEsc)));

										switch (c) {
											case '\0':
												SLKC_RETURN_IF_FALSE(writer->write("\\0"));
												break;
											case '\n':
												SLKC_RETURN_IF_FALSE(writer->write("\\n"));
												break;
											case '\t':
												SLKC_RETURN_IF_FALSE(writer->write("\\t"));
												break;
											case '\v':
												SLKC_RETURN_IF_FALSE(writer->write("\\v"));
												break;
											case '\f':
												SLKC_RETURN_IF_FALSE(writer->write("\\f"));
												break;
											case '\a':
												SLKC_RETURN_IF_FALSE(writer->write("\\a"));
												break;
											case '\b':
												SLKC_RETURN_IF_FALSE(writer->write("\\b"));
												break;
											case '\r':
												SLKC_RETURN_IF_FALSE(writer->write("\\r"));
												break;
											case '"':
												SLKC_RETURN_IF_FALSE(writer->write("\\\""));
												break;
											case '\\':
												SLKC_RETURN_IF_FALSE(writer->write("\\\\"));
												break;
										}

										idxCharSinceLastEsc = i + 1;
										break;

									default:

										break;
								}
							}

							if (idxCharSinceLastEsc < len)
								SLKC_RETURN_IF_FALSE(writer->write(std::string_view(data + idxCharSinceLastEsc, len - idxCharSinceLastEsc)));

							SLKC_RETURN_IF_FALSE(writer->write("\""));
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj));
							break;
						}
						case slake::ObjectKind::Array: {
							slake::ArrayObject *a = (slake::ArrayObject *)obj;

							SLKC_RETURN_IF_FALSE(writer->write("{ "));

							for (size_t i = 0; i < a->length; ++i) {
								if (i) {
									SLKC_RETURN_IF_FALSE(writer->write(", "));
								}

								slake::Reference rer = slake::Reference::makeArrayElementRef(a, i);
								slake::Value data;

								a->associatedRuntime->readVar(rer, data);

								SLKC_RETURN_IF_FALSE(decompileValue(allocator, writer, data));
							}

							SLKC_RETURN_IF_FALSE(writer->write(" }"));
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
			char s[19];
			sprintf(s, "%%%u", value.getRegIndex());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::TypeName: {
			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, value.getTypeName()));
			break;
		}
		default:
			std::terminate();
	}

	return true;
}

SLKC_API bool Decompiler::decompileIdRefEntries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &idRefIn) {
	for (size_t i = 0; i < idRefIn.size(); ++i) {
		if (i) {
			SLKC_RETURN_IF_FALSE(writer->write("."));
		}

		auto &curEntry = idRefIn.at(i);

		SLKC_RETURN_IF_FALSE(writer->write(curEntry.name));

		if (curEntry.genericArgs.size()) {
			SLKC_RETURN_IF_FALSE(writer->write("<"));
			for (size_t j = 0; j < curEntry.genericArgs.size(); ++j) {
				if (j) {
					SLKC_RETURN_IF_FALSE(writer->write(","));
				}
				SLKC_RETURN_IF_FALSE(decompileValue(allocator, writer, curEntry.genericArgs.at(j)));
			}
			SLKC_RETURN_IF_FALSE(writer->write(">"));
		}
	}

	return true;
}

SLKC_API bool Decompiler::decompileIdRef(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *idRefIn) {
	SLKC_RETURN_IF_FALSE(decompileIdRefEntries(allocator, writer, idRefIn->entries));

	if (idRefIn->paramTypes.hasValue()) {
		auto &paramTypes = *idRefIn->paramTypes;

		if (paramTypes.size()) {
			SLKC_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < paramTypes.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, paramTypes.at(i)));
			}

			if (idRefIn->hasVarArgs) {
				SLKC_RETURN_IF_FALSE(writer->write(", ..."));
			}

			SLKC_RETURN_IF_FALSE(writer->write(")"));
		} else {
			if (idRefIn->hasVarArgs) {
				SLKC_RETURN_IF_FALSE(writer->write("(...)"));
			} else {
				SLKC_RETURN_IF_FALSE(writer->write("()"));
			}
		}
	} else {
		if (idRefIn->hasVarArgs) {
			SLKC_RETURN_IF_FALSE(writer->write("(...)"));
		}
	}

	if (idRefIn->overridenType) {
		SLKC_RETURN_IF_FALSE(writer->write(" override "));
		SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, idRefIn->overridenType));
	}

	return true;
}

SLKC_API bool Decompiler::decompileModuleMembers(peff::Alloc *allocator, DumpWriter *writer, slake::BasicModuleObject *moduleObject, size_t indentLevel) {
	for (auto &i : moduleObject->fieldRecords) {
		for (size_t j = 0; j < indentLevel; ++j) {
			SLKC_RETURN_IF_FALSE(writer->write("\t"));
		}
		SLKC_RETURN_IF_FALSE(writer->write("let "));
		SLKC_RETURN_IF_FALSE(writer->write(i.name));
		SLKC_RETURN_IF_FALSE(writer->write(" "));
		SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, i.type));
		SLKC_RETURN_IF_FALSE(writer->write("\n"));
	}

	for (auto [k, v] : moduleObject->members) {
		switch (v->getObjectKind()) {
			case slake::ObjectKind::Fn: {
				slake::FnObject *obj = (slake::FnObject *)v;

				for (auto i : obj->overloadings) {
					for (size_t j = 0; j < indentLevel; ++j) {
						SLKC_RETURN_IF_FALSE(writer->write("\t"));
					}
					SLKC_RETURN_IF_FALSE(writer->write("fn "));

					SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, i.second->returnType));

					SLKC_RETURN_IF_FALSE(writer->write(" "));

					SLKC_RETURN_IF_FALSE(writer->write(obj->getName()));

					if (i.second->genericParams.size()) {
						SLKC_RETURN_IF_FALSE(writer->write("<"));

						for (size_t j = 0; j < i.second->genericParams.size(); ++j) {
							if (j) {
								SLKC_RETURN_IF_FALSE(writer->write(", "));
							}
							SLKC_RETURN_IF_FALSE(decompileGenericParam(allocator, writer, i.second->genericParams.at(j)));
						}

						SLKC_RETURN_IF_FALSE(writer->write(">"));
					}

					SLKC_RETURN_IF_FALSE(writer->write("("));

					for (size_t j = 0; j < i.second->paramTypes.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, i.second->paramTypes.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(")"));

					switch (i.second->overloadingKind) {
						case slake::FnOverloadingKind::Regular: {
							slake::RegularFnOverloadingObject *ol = (slake::RegularFnOverloadingObject *)i.second;

							if (dumpCfg) {
								slake::InternalExceptionPointer e;
								slake::opti::ControlFlowGraph cfg(allocator);

								if ((e = slake::opti::divideInstructionsIntoBasicBlocks(allocator, ol, allocator, cfg))) {
									std::terminate();
								}

								SLKC_RETURN_IF_FALSE(writer->write(" [\n"));

								for (size_t j = 0; j < cfg.basicBlocks.size(); ++j) {
									for (size_t k = 0; k < indentLevel; ++k) {
										SLKC_RETURN_IF_FALSE(writer->write("\t"));
									}

									{
										char s[18];

										sprintf(s, "block_%u:", (uint32_t)j);
										SLKC_RETURN_IF_FALSE(writer->write(s));
										SLKC_RETURN_IF_FALSE(writer->write("\n"));
									}

									const auto &curBasicBlock = cfg.basicBlocks.at(j);
									for (const slake::Instruction &curIns : curBasicBlock.instructions) {
										for (size_t j = 0; j < indentLevel + 1; ++j) {
											SLKC_RETURN_IF_FALSE(writer->write("\t"));
										}

										slake::slxfmt::SourceLocDesc *sld = nullptr;

										if (curIns.offSourceLocDesc != UINT32_MAX)
											sld = &ol->sourceLocDescs.at(curIns.offSourceLocDesc);

										if (sld) {
											char s[33];

											sprintf(s, "(%u %u): ", sld->line, sld->column);

											SLKC_RETURN_IF_FALSE(writer->write(s));
										}

										if (curIns.output != UINT32_MAX) {
											char s[9];

											sprintf(s, "%%%u = ", curIns.output);

											SLKC_RETURN_IF_FALSE(writer->write(s));
										}

										{
											const char *mnemonic = getMnemonicName(curIns.opcode);
											if (mnemonic) {
												SLKC_RETURN_IF_FALSE(writer->write(mnemonic));
												SLKC_RETURN_IF_FALSE(writer->write(" "));
											} else {
												char s[6];

												sprintf(s, "0x%0.2x ", (int)curIns.opcode);

												SLKC_RETURN_IF_FALSE(writer->write(s));
											}
										}

										for (size_t k = 0; k < curIns.nOperands; ++k) {
											if (k) {
												SLKC_RETURN_IF_FALSE(writer->write(", "));
											}
											if (curIns.operands[k].valueType == slake::ValueType::Label) {
												char s[18];

												sprintf(s, "#block_%u", (uint32_t)curIns.operands[k].getLabel());
												SLKC_RETURN_IF_FALSE(writer->write(s));
											} else
												SLKC_RETURN_IF_FALSE(decompileValue(allocator, writer, curIns.operands[k]));
										}

										SLKC_RETURN_IF_FALSE(writer->write("\n"));
									}
								}

								for (size_t j = 0; j < indentLevel; ++j) {
									SLKC_RETURN_IF_FALSE(writer->write("\t"));
								}

								SLKC_RETURN_IF_FALSE(writer->write("]\n"));
							} else {
								SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

								for (size_t j = 0; j < ol->instructions.size(); ++j) {
									auto &curIns = ol->instructions.at(j);
									for (size_t k = 0; k < indentLevel + 1; ++k) {
										SLKC_RETURN_IF_FALSE(writer->write("\t"));
									}

									slake::slxfmt::SourceLocDesc *sld = nullptr;

									if (curIns.offSourceLocDesc != UINT32_MAX)
										sld = &ol->sourceLocDescs.at(curIns.offSourceLocDesc);

									if (sld) {
										char s[23];

										sprintf(s, "(%u, %u): ", sld->line, sld->column);

										SLKC_RETURN_IF_FALSE(writer->write(s));
									}

									if (curIns.output != UINT32_MAX) {
										char s[16];

										sprintf(s, "%%%u = ", curIns.output);

										SLKC_RETURN_IF_FALSE(writer->write(s));
									}

									{
										const char *mnemonic = getMnemonicName(curIns.opcode);
										if (mnemonic) {
											SLKC_RETURN_IF_FALSE(writer->write(mnemonic));
											SLKC_RETURN_IF_FALSE(writer->write(" "));
										} else {
											char s[6];

											sprintf(s, "0x%0.2x ", (int)curIns.opcode);

											SLKC_RETURN_IF_FALSE(writer->write(s));
										}
									}

									for (size_t k = 0; k < curIns.nOperands; ++k) {
										if (k) {
											SLKC_RETURN_IF_FALSE(writer->write(", "));
										}
										SLKC_RETURN_IF_FALSE(decompileValue(allocator, writer, curIns.operands[k]));
									}

									SLKC_RETURN_IF_FALSE(writer->write("\n"));
								}

								for (size_t j = 0; j < indentLevel; ++j) {
									SLKC_RETURN_IF_FALSE(writer->write("\t"));
								}

								SLKC_RETURN_IF_FALSE(writer->write("}\n"));
							}

							break;
						}
						default:
							SLKC_RETURN_IF_FALSE(writer->write(";"));
							break;
					}
				}
				break;
			}
			case slake::ObjectKind::Class: {
				slake::ClassObject *obj = (slake::ClassObject *)v;

				for (size_t j = 0; j < indentLevel; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("class "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->getName()));

				if (obj->genericParams.size()) {
					SLKC_RETURN_IF_FALSE(writer->write("<"));

					for (size_t j = 0; j < obj->genericParams.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompileGenericParam(allocator, writer, obj->genericParams.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(">"));
				}

				SLKC_RETURN_IF_FALSE(writer->write(" "));

				if (obj->baseType)
					SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->baseType));

				if (obj->implTypes.size()) {
					SLKC_RETURN_IF_FALSE(writer->write(": "));
					for (size_t i = 0; i < obj->implTypes.size(); ++i) {
						if (i) {
							SLKC_RETURN_IF_FALSE(writer->write(" + "));
						}

						SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->implTypes.at(i)));
					}
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				SLKC_RETURN_IF_FALSE(decompileModuleMembers(allocator, writer, obj, indentLevel + 1));

				for (size_t j = 0; j < indentLevel; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("}\n"));
				break;
			}
			case slake::ObjectKind::Interface: {
				slake::InterfaceObject *obj = (slake::InterfaceObject *)v;

				for (size_t j = 0; j < indentLevel; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("interface "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->getName()));

				if (obj->genericParams.size()) {
					SLKC_RETURN_IF_FALSE(writer->write("<"));

					for (size_t j = 0; j < obj->genericParams.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompileGenericParam(allocator, writer, obj->genericParams.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(">"));
				}

				SLKC_RETURN_IF_FALSE(writer->write(" "));

				if (obj->implTypes.size()) {
					SLKC_RETURN_IF_FALSE(writer->write(": "));
					for (size_t i = 0; i < obj->implTypes.size(); ++i) {
						if (i) {
							SLKC_RETURN_IF_FALSE(writer->write(" + "));
						}

						SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->implTypes.at(i)));
					}
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				SLKC_RETURN_IF_FALSE(decompileModuleMembers(allocator, writer, obj, indentLevel + 1));

				for (size_t j = 0; j < indentLevel; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("}\n"));
				break;
			}
			case slake::ObjectKind::Struct: {
				slake::StructObject *obj = (slake::StructObject *)v;

				for (size_t j = 0; j < indentLevel; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("struct "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->getName()));

				if (obj->genericParams.size()) {
					SLKC_RETURN_IF_FALSE(writer->write("<"));

					for (size_t j = 0; j < obj->genericParams.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompileGenericParam(allocator, writer, obj->genericParams.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(">"));
				}

				if (obj->implTypes.size()) {
					SLKC_RETURN_IF_FALSE(writer->write(": "));
					for (size_t i = 0; i < obj->implTypes.size(); ++i) {
						if (i) {
							SLKC_RETURN_IF_FALSE(writer->write(" + "));
						}

						SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, obj->implTypes.at(i)));
					}
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				SLKC_RETURN_IF_FALSE(decompileModuleMembers(allocator, writer, obj, indentLevel + 1));

				for (size_t j = 0; j < indentLevel; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("}\n"));
				break;
			}
			default:
				break;
		}
	}

	return true;
}

SLKC_API bool Decompiler::decompileModule(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *moduleObject) {
	slake::Runtime *runtime = moduleObject->associatedRuntime;

	{
		peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

		if (!runtime->getFullRef(allocator, moduleObject, moduleFullName))
			return false;

		SLKC_RETURN_IF_FALSE(writer->write("module "));
		SLKC_RETURN_IF_FALSE(decompileIdRefEntries(allocator, writer, moduleFullName));
		SLKC_RETURN_IF_FALSE(writer->write(";\n"));
	}

	SLKC_RETURN_IF_FALSE(decompileModuleMembers(allocator, writer, moduleObject));

	return true;
}
