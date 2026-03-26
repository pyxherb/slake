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

SLKC_API const char *slkc::get_mnemonic_name(slake::Opcode opcode) {
	switch (opcode) {
		MNEMONIC_NAME_CASE(INVALID)
		MNEMONIC_NAME_CASE(LOAD)
		MNEMONIC_NAME_CASE(RLOAD)
		MNEMONIC_NAME_CASE(STORE)
		MNEMONIC_NAME_CASE(COPYI8)
		MNEMONIC_NAME_CASE(COPYI16)
		MNEMONIC_NAME_CASE(COPYI32)
		MNEMONIC_NAME_CASE(COPYI64)
		MNEMONIC_NAME_CASE(COPYISIZE)
		MNEMONIC_NAME_CASE(COPYU8)
		MNEMONIC_NAME_CASE(COPYU16)
		MNEMONIC_NAME_CASE(COPYU32)
		MNEMONIC_NAME_CASE(COPYU64)
		MNEMONIC_NAME_CASE(COPYUSIZE)
		MNEMONIC_NAME_CASE(COPYF32)
		MNEMONIC_NAME_CASE(COPYF64)
		MNEMONIC_NAME_CASE(COPYBOOL)
		MNEMONIC_NAME_CASE(COPYNULL)
		MNEMONIC_NAME_CASE(COPY)
		MNEMONIC_NAME_CASE(LARG)
		MNEMONIC_NAME_CASE(LARGV)
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
		MNEMONIC_NAME_CASE(LCURFN)
		MNEMONIC_NAME_CASE(JMP)
		MNEMONIC_NAME_CASE(BR)
		MNEMONIC_NAME_CASE(PHI)
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
		MNEMONIC_NAME_CASE(DCMT)
		MNEMONIC_NAME_CASE(TYPEOF)
		MNEMONIC_NAME_CASE(CONSTSW)
		default:
			return nullptr;
	}
	std::terminate();
}

SLKC_API Decompiler::Decompiler() {
}

SLKC_API Decompiler::~Decompiler() {
}

SLKC_API bool Decompiler::decompile_generic_param(peff::Alloc *allocator, DumpWriter *writer, const slake::GenericParam &generic_param) {
	SLKC_RETURN_IF_FALSE(writer->write(generic_param.name));
	if (generic_param.input_type != slake::TypeId::Invalid) {
		SLKC_RETURN_IF_FALSE(writer->write(" as "));
		SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, generic_param.input_type));
		if ((generic_param.base_type != slake::TypeId::Invalid) || (generic_param.interfaces.size()))
			SLKC_RETURN_IF_FALSE(writer->write("/* With extraneous constraints */"));
	} else {
		if (generic_param.base_type != slake::TypeId::Invalid) {
			SLKC_RETURN_IF_FALSE(writer->write("("));
			SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, generic_param.base_type));
			SLKC_RETURN_IF_FALSE(writer->write(")"));
		}
		if (generic_param.interfaces.size()) {
			SLKC_RETURN_IF_FALSE(writer->write(": "));
			for (size_t i = 0; i < generic_param.interfaces.size(); ++i) {
				if (i)
					SLKC_RETURN_IF_FALSE(writer->write(" + "));
				SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, generic_param.interfaces.at(i)));
			}
		}
	}

	return true;
}

SLKC_API bool Decompiler::decompile_type_name(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type) {
	switch (type.type_id) {
		case slake::TypeId::Invalid:
			SLKC_RETURN_IF_FALSE(writer->write("/* Invalid type */"));
			break;
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
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					SLKC_RETURN_IF_FALSE(writer->write("@"));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("@"));
					SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::StructInstance: {
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Struct: {
					SLKC_RETURN_IF_FALSE(writer->write("struct "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("struct "));
					SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::ScopedEnum: {
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Struct: {
					SLKC_RETURN_IF_FALSE(writer->write("enum base "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum base "));
					SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::TypelessScopedEnum: {
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Struct: {
					SLKC_RETURN_IF_FALSE(writer->write("enum "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum "));
					SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::UnionEnum: {
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Struct: {
					SLKC_RETURN_IF_FALSE(writer->write("enum union "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum union "));
					SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::UnionEnumItem: {
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Struct: {
					SLKC_RETURN_IF_FALSE(writer->write("enum struct "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_FALSE(writer->write("enum struct "));
					SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::GenericArg: {
			auto obj = type.get_generic_arg_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			SLKC_RETURN_IF_FALSE(writer->write("@!"));
			SLKC_RETURN_IF_FALSE(writer->write(obj->name_object->data.data(), obj->name_object->data.size()));
			break;
		}
		case slake::TypeId::Array: {
			auto obj = type.get_array_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->element_type->type_ref));
			SLKC_RETURN_IF_FALSE(writer->write("[]"));
			break;
		}
		case slake::TypeId::Ref: {
			auto obj = type.get_ref_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->referenced_type->type_ref));
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
			auto obj = type.get_param_type_list_type_def();

			SLKC_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < obj->param_types.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->param_types.at(i)->type_ref));
			}

			SLKC_RETURN_IF_FALSE(writer->write(")"));
			break;
		}
		case slake::TypeId::Tuple: {
			auto obj = type.get_tuple_type_def();

			SLKC_RETURN_IF_FALSE(writer->write("["));

			for (size_t i = 0; i < obj->element_types.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->element_types.at(i)->type_ref));
			}

			SLKC_RETURN_IF_FALSE(writer->write("]"));
			break;
		}
		case slake::TypeId::SIMD: {
			auto obj = type.get_simdtype_def();

			SLKC_RETURN_IF_FALSE(writer->write("simd_t<"));

			SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->type->type_ref));

			SLKC_RETURN_IF_FALSE(writer->write(", "));

			char s[16];
			sprintf(s, "%u", obj->width);
			SLKC_RETURN_IF_FALSE(writer->write(s));

			SLKC_RETURN_IF_FALSE(writer->write(">"));
			break;
		}
		case slake::TypeId::Unpacking: {
			auto obj = type.get_unpacking_type_def();
			SLKC_RETURN_IF_FALSE(writer->write("@..."));
			SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->type->type_ref));
			break;
		}
		default:
			std::terminate();
	}

	if (type.is_nullable())
		SLKC_RETURN_IF_FALSE(writer->write("?"));
	return true;
}

SLKC_API bool Decompiler::decompile_value(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value) {
	switch (value.value_type) {
		case slake::ValueType::I8: {
			char s[8];
			sprintf(s, "%hd", (int16_t)value.get_i8());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I16: {
			char s[16];
			sprintf(s, "%hd", (int16_t)value.get_i16());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I32: {
			char s[32];
			sprintf(s, "%d", value.get_i32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I64: {
			char s[48];
			sprintf(s, "%lld", value.get_i64());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U8: {
			char s[4];
			sprintf(s, "%hu", (uint16_t)value.get_u8());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U16: {
			char s[8];
			sprintf(s, "%hu", (uint16_t)value.get_u16());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U32: {
			char s[16];
			sprintf(s, "%u", value.get_u32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U64: {
			char s[32];
			sprintf(s, "%llu", value.get_u64());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F32: {
			char s[16];
			sprintf(s, "%f", value.get_f32());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F64: {
			char s[32];
			sprintf(s, "%f", value.get_f64());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::Bool:
			SLKC_RETURN_IF_FALSE(writer->write(value.get_bool() ? "true" : "false"));
			break;
		case slake::ValueType::Reference: {
			const slake::Reference &er = value.get_reference();

			switch (er.kind) {
				case slake::ReferenceKind::ObjectRef: {
					slake::Object *obj = er.as_object;

					if (!obj) {
						SLKC_RETURN_IF_FALSE(writer->write("null"));
						break;
					}

					switch (obj->get_object_kind()) {
						case slake::ObjectKind::String: {
							SLKC_RETURN_IF_FALSE(writer->write("\""));

							slake::StringObject *s = (slake::StringObject *)obj;

							const char *data = s->data.data();
							size_t len = s->data.size();
							char c;

							size_t idx_char_since_last_esc = 0;

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
										SLKC_RETURN_IF_FALSE(writer->write(std::string_view(data + idx_char_since_last_esc, i - idx_char_since_last_esc)));

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

										idx_char_since_last_esc = i + 1;
										break;

									default:

										break;
								}
							}

							if (idx_char_since_last_esc < len)
								SLKC_RETURN_IF_FALSE(writer->write(std::string_view(data + idx_char_since_last_esc, len - idx_char_since_last_esc)));

							SLKC_RETURN_IF_FALSE(writer->write("\""));
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLKC_RETURN_IF_FALSE(decompile_id_ref(allocator, writer, (slake::IdRefObject *)obj));
							break;
						}
						case slake::ObjectKind::Array: {
							slake::ArrayObject *a = (slake::ArrayObject *)obj;

							SLKC_RETURN_IF_FALSE(writer->write("{ "));

							for (size_t i = 0; i < a->length; ++i) {
								if (i) {
									SLKC_RETURN_IF_FALSE(writer->write(", "));
								}

								slake::Reference rer = slake::ArrayElementRef(a, i);
								slake::Value data;

								slake::Runtime::read_var(rer, data);

								SLKC_RETURN_IF_FALSE(decompile_value(allocator, writer, data));
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
			char s[32];
			sprintf(s, "%%%u", value.get_reg_index());
			SLKC_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::TypeName: {
			SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, value.get_type_name()));
			break;
		}
		default:
			std::terminate();
	}

	return true;
}

SLKC_API bool Decompiler::decompile_id_ref_entries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &id_ref_in) {
	for (size_t i = 0; i < id_ref_in.size(); ++i) {
		if (i) {
			SLKC_RETURN_IF_FALSE(writer->write("."));
		}

		auto &cur_entry = id_ref_in.at(i);

		SLKC_RETURN_IF_FALSE(writer->write(cur_entry.name));

		if (cur_entry.generic_args.size()) {
			SLKC_RETURN_IF_FALSE(writer->write("<"));
			for (size_t j = 0; j < cur_entry.generic_args.size(); ++j) {
				if (j) {
					SLKC_RETURN_IF_FALSE(writer->write(","));
				}
				SLKC_RETURN_IF_FALSE(decompile_value(allocator, writer, cur_entry.generic_args.at(j)));
			}
			SLKC_RETURN_IF_FALSE(writer->write(">"));
		}
	}

	return true;
}

SLKC_API bool Decompiler::decompile_id_ref(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *id_ref_in) {
	SLKC_RETURN_IF_FALSE(decompile_id_ref_entries(allocator, writer, id_ref_in->entries));

	if (id_ref_in->param_types.has_value()) {
		auto &param_types = *id_ref_in->param_types;

		if (param_types.size()) {
			SLKC_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < param_types.size(); ++i) {
				if (i) {
					SLKC_RETURN_IF_FALSE(writer->write(", "));
				}

				SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, param_types.at(i)));
			}

			if (id_ref_in->has_var_args) {
				SLKC_RETURN_IF_FALSE(writer->write(", ..."));
			}

			SLKC_RETURN_IF_FALSE(writer->write(")"));
		} else {
			if (id_ref_in->has_var_args) {
				SLKC_RETURN_IF_FALSE(writer->write("(...)"));
			} else {
				SLKC_RETURN_IF_FALSE(writer->write("()"));
			}
		}
	} else {
		if (id_ref_in->has_var_args) {
			SLKC_RETURN_IF_FALSE(writer->write("(...)"));
		}
	}

	if (id_ref_in->overriden_type) {
		SLKC_RETURN_IF_FALSE(writer->write(" override "));
		SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, id_ref_in->overriden_type));
	}

	return true;
}

SLKC_API bool Decompiler::decompile_module_members(peff::Alloc *allocator, DumpWriter *writer, slake::BasicModuleObject *module_object, size_t indent_level) {
	for (auto &i : module_object->get_field_records()) {
		for (size_t j = 0; j < indent_level; ++j) {
			SLKC_RETURN_IF_FALSE(writer->write("\t"));
		}
		SLKC_RETURN_IF_FALSE(writer->write("let "));
		SLKC_RETURN_IF_FALSE(writer->write(i.name));
		SLKC_RETURN_IF_FALSE(writer->write(" "));
		SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, i.type));
		SLKC_RETURN_IF_FALSE(writer->write("\n"));
	}

	for (auto [k, v] : module_object->get_members()) {
		switch (v->get_object_kind()) {
			case slake::ObjectKind::Fn: {
				slake::FnObject *obj = (slake::FnObject *)v;

				for (auto i : obj->overloadings) {
					for (size_t j = 0; j < indent_level; ++j) {
						SLKC_RETURN_IF_FALSE(writer->write("\t"));
					}
					SLKC_RETURN_IF_FALSE(writer->write("fn "));

					SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, i.second->return_type));

					SLKC_RETURN_IF_FALSE(writer->write(" "));

					SLKC_RETURN_IF_FALSE(writer->write(obj->get_name()));

					if (i.second->generic_params.size()) {
						SLKC_RETURN_IF_FALSE(writer->write("<"));

						for (size_t j = 0; j < i.second->generic_params.size(); ++j) {
							if (j) {
								SLKC_RETURN_IF_FALSE(writer->write(", "));
							}
							SLKC_RETURN_IF_FALSE(decompile_generic_param(allocator, writer, i.second->generic_params.at(j)));
						}

						SLKC_RETURN_IF_FALSE(writer->write(">"));
					}

					SLKC_RETURN_IF_FALSE(writer->write("("));

					for (size_t j = 0; j < i.second->param_types.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, i.second->param_types.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(")"));

					switch (i.second->overloading_kind) {
						case slake::FnOverloadingKind::Regular: {
							slake::RegularFnOverloadingObject *ol = (slake::RegularFnOverloadingObject *)i.second;

							if (dump_cfg) {
								slake::InternalExceptionPointer e;
								slake::opti::ControlFlowGraph cfg(allocator);

								if ((e = slake::opti::divide_instructions_into_basic_blocks(allocator, ol, allocator, cfg))) {
									std::terminate();
								}

								SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

								for (size_t j = 0; j < cfg.basic_blocks.size(); ++j) {
									for (size_t k = 0; k < indent_level; ++k) {
										SLKC_RETURN_IF_FALSE(writer->write("\t"));
									}

									{
										char s[36];

										sprintf(s, "block_%u:", (uint32_t)j);
										SLKC_RETURN_IF_FALSE(writer->write(s));
										SLKC_RETURN_IF_FALSE(writer->write("\n"));
									}

									const auto &cur_basic_block = cfg.basic_blocks.at(j);
									for (const slake::Instruction &cur_ins : cur_basic_block.instructions) {
										for (size_t j = 0; j < indent_level + 1; ++j) {
											SLKC_RETURN_IF_FALSE(writer->write("\t"));
										}

										slake::slxfmt::SourceLocDesc *sld = nullptr;

										if (cur_ins.off_source_loc_desc != UINT32_MAX)
											sld = &ol->source_loc_descs.at(cur_ins.off_source_loc_desc);

										if (sld) {
											char s[42];

											sprintf(s, "(%u %u): ", sld->line, sld->column);

											SLKC_RETURN_IF_FALSE(writer->write(s));
										}

										if (cur_ins.output != UINT32_MAX) {
											char s[32];

											sprintf(s, "%%%u = ", cur_ins.output);

											SLKC_RETURN_IF_FALSE(writer->write(s));
										}

										{
											const char *mnemonic = get_mnemonic_name(cur_ins.opcode);
											if (mnemonic) {
												SLKC_RETURN_IF_FALSE(writer->write(mnemonic));
												SLKC_RETURN_IF_FALSE(writer->write(" "));
											} else {
												char s[13];

												sprintf(s, "0x%0.2x ", (int)cur_ins.opcode);

												SLKC_RETURN_IF_FALSE(writer->write(s));
											}
										}

										for (size_t k = 0; k < cur_ins.num_operands; ++k) {
											if (k) {
												SLKC_RETURN_IF_FALSE(writer->write(", "));
											}
											if (cur_ins.operands[k].value_type == slake::ValueType::Label) {
												char s[18];

												sprintf(s, "#block_%u", (uint32_t)cur_ins.operands[k].get_label());
												SLKC_RETURN_IF_FALSE(writer->write(s));
											} else
												SLKC_RETURN_IF_FALSE(decompile_value(allocator, writer, cur_ins.operands[k]));
										}

										SLKC_RETURN_IF_FALSE(writer->write("\n"));
									}
								}

								for (size_t j = 0; j < indent_level; ++j) {
									SLKC_RETURN_IF_FALSE(writer->write("\t"));
								}

								SLKC_RETURN_IF_FALSE(writer->write("}\n"));
							} else {
								SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

								size_t block_depth = 0;
								for (size_t j = 0; j < ol->instructions.size(); ++j) {
									auto &cur_ins = ol->instructions.at(j);

									if((cur_ins.opcode == slake::Opcode::LEAVE) && block_depth)
										--block_depth;

									for (size_t k = 0; k < indent_level + 1 + block_depth; ++k) {
										SLKC_RETURN_IF_FALSE(writer->write("\t"));
									}

									if(cur_ins.opcode == slake::Opcode::ENTER)
										++block_depth;

									slake::slxfmt::SourceLocDesc *sld = nullptr;

									if (cur_ins.off_source_loc_desc != UINT32_MAX)
										sld = &ol->source_loc_descs.at(cur_ins.off_source_loc_desc);

									if (sld) {
										char s[32];

										sprintf(s, "(%u, %u): ", sld->line, sld->column);

										SLKC_RETURN_IF_FALSE(writer->write(s));
									}

									if (cur_ins.output != UINT32_MAX) {
										char s[32];

										sprintf(s, "%%%u = ", cur_ins.output);

										SLKC_RETURN_IF_FALSE(writer->write(s));
									}

									{
										const char *mnemonic = get_mnemonic_name(cur_ins.opcode);
										if (mnemonic) {
											SLKC_RETURN_IF_FALSE(writer->write(mnemonic));
											SLKC_RETURN_IF_FALSE(writer->write(" "));
										} else {
											char s[13];

											sprintf(s, "0x%0.2x ", (int)cur_ins.opcode);

											SLKC_RETURN_IF_FALSE(writer->write(s));
										}
									}

									for (size_t k = 0; k < cur_ins.num_operands; ++k) {
										if (k) {
											SLKC_RETURN_IF_FALSE(writer->write(", "));
										}
										SLKC_RETURN_IF_FALSE(decompile_value(allocator, writer, cur_ins.operands[k]));
									}

									SLKC_RETURN_IF_FALSE(writer->write("\n"));
								}

								for (size_t j = 0; j < indent_level; ++j) {
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

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("class "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->get_name()));

				if (obj->generic_params.size()) {
					SLKC_RETURN_IF_FALSE(writer->write("<"));

					for (size_t j = 0; j < obj->generic_params.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompile_generic_param(allocator, writer, obj->generic_params.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(">"));
				}

				SLKC_RETURN_IF_FALSE(writer->write(" "));

				if (obj->base_type)
					SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->base_type));

				if (obj->impl_types.size()) {
					SLKC_RETURN_IF_FALSE(writer->write(": "));
					for (size_t i = 0; i < obj->impl_types.size(); ++i) {
						if (i) {
							SLKC_RETURN_IF_FALSE(writer->write(" + "));
						}

						SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->impl_types.at(i)));
					}
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				SLKC_RETURN_IF_FALSE(decompile_module_members(allocator, writer, obj, indent_level + 1));

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("}\n"));
				break;
			}
			case slake::ObjectKind::Interface: {
				slake::InterfaceObject *obj = (slake::InterfaceObject *)v;

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("interface "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->get_name()));

				if (obj->generic_params.size()) {
					SLKC_RETURN_IF_FALSE(writer->write("<"));

					for (size_t j = 0; j < obj->generic_params.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompile_generic_param(allocator, writer, obj->generic_params.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(">"));
				}

				SLKC_RETURN_IF_FALSE(writer->write(" "));

				if (obj->impl_types.size()) {
					SLKC_RETURN_IF_FALSE(writer->write(": "));
					for (size_t i = 0; i < obj->impl_types.size(); ++i) {
						if (i) {
							SLKC_RETURN_IF_FALSE(writer->write(" + "));
						}

						SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->impl_types.at(i)));
					}
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				SLKC_RETURN_IF_FALSE(decompile_module_members(allocator, writer, obj, indent_level + 1));

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("}\n"));
				break;
			}
			case slake::ObjectKind::Struct: {
				slake::StructObject *obj = (slake::StructObject *)v;

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("struct "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->get_name()));

				if (obj->generic_params.size()) {
					SLKC_RETURN_IF_FALSE(writer->write("<"));

					for (size_t j = 0; j < obj->generic_params.size(); ++j) {
						if (j) {
							SLKC_RETURN_IF_FALSE(writer->write(", "));
						}
						SLKC_RETURN_IF_FALSE(decompile_generic_param(allocator, writer, obj->generic_params.at(j)));
					}

					SLKC_RETURN_IF_FALSE(writer->write(">"));
				}

				if (obj->impl_types.size()) {
					SLKC_RETURN_IF_FALSE(writer->write(": "));
					for (size_t i = 0; i < obj->impl_types.size(); ++i) {
						if (i) {
							SLKC_RETURN_IF_FALSE(writer->write(" + "));
						}

						SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->impl_types.at(i)));
					}
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				SLKC_RETURN_IF_FALSE(decompile_module_members(allocator, writer, obj, indent_level + 1));

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("}\n"));
				break;
			}
			case slake::ObjectKind::ScopedEnum: {
				slake::ScopedEnumObject *obj = (slake::ScopedEnumObject *)v;

				for (size_t j = 0; j < indent_level; ++j) {
					SLKC_RETURN_IF_FALSE(writer->write("\t"));
				}

				SLKC_RETURN_IF_FALSE(writer->write("enum "));

				SLKC_RETURN_IF_FALSE(writer->write(obj->get_name()));

				if (obj->base_type) {
					SLKC_RETURN_IF_FALSE(writer->write("("));
					SLKC_RETURN_IF_FALSE(decompile_type_name(allocator, writer, obj->base_type));
					SLKC_RETURN_IF_FALSE(writer->write(")"));
				}

				SLKC_RETURN_IF_FALSE(writer->write(" {\n"));

				size_t k = 0;
				for (auto &j : obj->get_field_records()) {
					for (size_t l = 0; l < indent_level + 1; ++l) {
						SLKC_RETURN_IF_FALSE(writer->write("\t"));
					}
					SLKC_RETURN_IF_FALSE(writer->write(j.name));

					if (obj->base_type) {
						SLKC_RETURN_IF_FALSE(writer->write(" = "));

						slake::Value data;
						slake::Runtime::read_var(slake::StaticFieldRef(obj, k), data);

						SLKC_RETURN_IF_FALSE(decompile_value(allocator, writer, data));
					}

					if (k + 1 < indent_level + 1)
						SLKC_RETURN_IF_FALSE(writer->write(","));
					SLKC_RETURN_IF_FALSE(writer->write("\n"));
					++k;
				}

				for (size_t j = 0; j < indent_level; ++j) {
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

SLKC_API bool Decompiler::decompile_module(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *module_object) {
	slake::Runtime *runtime = module_object->associated_runtime;

	{
		peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

		if (!runtime->get_full_ref(allocator, module_object, module_full_name))
			return false;

		SLKC_RETURN_IF_FALSE(writer->write("module "));
		SLKC_RETURN_IF_FALSE(decompile_id_ref_entries(allocator, writer, module_full_name));
		SLKC_RETURN_IF_FALSE(writer->write(";\n"));
	}

	SLKC_RETURN_IF_FALSE(decompile_module_members(allocator, writer, module_object));

	return true;
}
