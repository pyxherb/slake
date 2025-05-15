#include "decompiler.h"

using namespace slkc;

#define SLKC_RETURN_IF_FALSE(e) \
	if (!e) return false

SLKC_API DumpWriter::~DumpWriter() {
}

SLKC_API bool slkc::decompileTypeName(peff::Alloc *allocator, DumpWriter *writer, const slake::Type &type) {
	switch (type.typeId) {
		case slake::TypeId::None:
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
			auto obj = type.getCustomTypeExData();

			slake::Runtime *runtime = obj->associatedRuntime;

			switch (obj->getKind()) {
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					SLKC_RETURN_IF_FALSE(writer->write("@"));

					peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

					if (!runtime->getFullRef(allocator, (slake::MemberObject *)obj, moduleFullName))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("@"));
					SLKC_RETURN_IF_FALSE(decompileIdRef(allocator, writer, (slake::IdRefObject *)obj));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::GenericArg: {
			auto obj = type.exData.genericArg.nameObject;

			SLKC_RETURN_IF_FALSE(writer->write("@!"));
			SLKC_RETURN_IF_FALSE(writer->write(obj->data.data(), obj->data.size()));
			break;
		}
		case slake::TypeId::Array: {
			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, type.getArrayExData()));
			SLKC_RETURN_IF_FALSE(writer->write("[]"));
			break;
		}
		case slake::TypeId::Ref: {
			SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, type.getArrayExData()));
			SLKC_RETURN_IF_FALSE(writer->write("&"));
			break;
		}
		case slake::TypeId::FnDelegate: {
			SLKC_RETURN_IF_FALSE(writer->write("(fn delegate, not implemented yet)"));
			break;
		}
		case slake::TypeId::Any:
			SLKC_RETURN_IF_FALSE(writer->write("any"));
			break;
		default:
			std::terminate();
	}

	return true;
}

SLKC_API bool slkc::decompileValue(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value) {
	switch (value.valueType) {
		case slake::ValueType::I8: {
			char s[4];
			sprintf(s, "%hd", (int16_t)value.getI8());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I16: {
			char s[8];
			sprintf(s, "%hd", (int16_t)value.getI16());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I32: {
			char s[16];
			sprintf(s, "%d", value.getI32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I64: {
			char s[32];
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
		case slake::ValueType::Bool:
			SLKC_RETURN_IF_FALSE(writer->write(value.getBool() ? "true" : "false"));
			break;
		case slake::ValueType::EntityRef: {
			const slake::EntityRef &er = value.getEntityRef();

			switch (er.kind) {
				case slake::ObjectRefKind::ObjectRef: {
					slake::Object *obj = er.asObject.instanceObject;

					switch (obj->getKind()) {
						case slake::ObjectKind::String: {
							SLKC_RETURN_IF_FALSE(writer->write("\""));

							SLKC_RETURN_IF_FALSE(writer->write(((slake::StringObject *)obj)->data));

							SLKC_RETURN_IF_FALSE(writer->write("\""));
							break;
						}
						case slake::ObjectKind::Ref: {
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

								slake::EntityRef rer = slake::EntityRef::makeArrayElementRef(a, i);

								SLKC_RETURN_IF_FALSE(decompileValue(allocator, writer, a->associatedRuntime->readVarUnsafe(rer)));
							}

							SLKC_RETURN_IF_FALSE(writer->write(" }"));
						}
					}
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::ValueType::RegRef: {
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

SLKC_API bool slkc::decompileIdRefEntries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &idRefIn) {
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
				SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, curEntry.genericArgs.at(j)));
			}
			SLKC_RETURN_IF_FALSE(writer->write(">"));
		}
	}

	return true;
}

SLKC_API bool slkc::decompileIdRef(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *idRefIn) {
	SLKC_RETURN_IF_FALSE(decompileIdRefEntries(allocator, writer, idRefIn->entries));

	if (idRefIn->paramTypes) {
		auto &paramTypes = *idRefIn->paramTypes;

		if (paramTypes.size()) {
			SLKC_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < paramTypes.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, paramTypes.at(i)));
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

	return true;
}

SLKC_API bool slkc::decompileModuleMembers(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *moduleObject, size_t indentLevel) {
	for (auto &i : moduleObject->fieldRecords) {
		SLKC_RETURN_IF_FALSE(writer->write("let "));
		SLKC_RETURN_IF_FALSE(writer->write(i.name));
		SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, i.type));
		SLKC_RETURN_IF_FALSE(writer->write("\n"));
	}

	for (auto [k, v] : moduleObject->members) {
		switch (v->getKind()) {
			case slake::ObjectKind::Fn: {
				slake::FnObject *obj = (slake::FnObject *)v;

				for (auto i : obj->overloadings) {
					for (size_t j = 0; j < indentLevel; ++j) {
						SLKC_RETURN_IF_FALSE(writer->write("\t"));
					}
					SLKC_RETURN_IF_FALSE(writer->write("fn "));

					SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, i->returnType));

					SLKC_RETURN_IF_FALSE(writer->write(" "));

					SLKC_RETURN_IF_FALSE(writer->write(obj->name));

					for (size_t j = 0; j < i->paramTypes.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompileTypeName(allocator, writer, i->paramTypes.at(j)));
					}

					switch (i->overloadingKind) {
						case slake::FnOverloadingKind::Regular: {
							slake::RegularFnOverloadingObject *ol = (slake::RegularFnOverloadingObject *)i;

							SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

							for (size_t j = 0; j < ol->instructions.size(); ++j) {
								auto &curIns = ol->instructions.at(j);
								for (size_t k = 0; k < indentLevel + 1; ++k) {
									SLKC_RETURN_IF_FALSE(writer->write("\t"));
								}

								if (curIns.output != UINT32_MAX)
								{
									char s[9];

									sprintf(s, "%%%u =  ", (int)curIns.output);

									SLKC_RETURN_IF_FALSE(writer->write(s));
								}

								{
									char s[6];

									sprintf(s, "0x%0.2x ", (int)curIns.opcode);

									SLKC_RETURN_IF_FALSE(writer->write(s));
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

				SLKC_RETURN_IF_FALSE(writer->write("class "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->name));

				SLKC_RETURN_IF_FALSE(writer->write(" "));

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

				SLKC_RETURN_IF_FALSE(writer->write("interface "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->name));

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
			default:
				break;
		}
	}

	return true;
}

SLKC_API bool slkc::decompileModule(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *moduleObject) {
	slake::Runtime *runtime = moduleObject->associatedRuntime;

	{
		peff::DynArray<slake::IdRefEntry> moduleFullName(allocator);

		if (!runtime->getFullRef(allocator, moduleObject, moduleFullName))
			return false;

		SLKC_RETURN_IF_FALSE(writer->write(".module "));
		SLKC_RETURN_IF_FALSE(decompileIdRefEntries(allocator, writer, moduleFullName));
	}

	SLKC_RETURN_IF_FALSE(decompileModuleMembers(allocator, writer, moduleObject));

	return true;
}
