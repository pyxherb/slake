#include "../compiler.h"

using namespace slkc;

SLKC_API Writer::~Writer() {
}

SLKC_API peff::Option<CompilationError> slkc::dump_generic_param(
	peff::Alloc *allocator,
	Writer *writer,
	const slake::GenericParam &generic_param) {
	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(generic_param.name.size()));
	SLKC_RETURN_IF_COMP_ERROR(writer->write(generic_param.name.data(), generic_param.name.size()));

	if (generic_param.input_type != slake::TypeId::Invalid) {
		SLKC_RETURN_IF_COMP_ERROR(writer->write_bool(true));

		SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, generic_param.input_type));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(writer->write_bool(false));

		bool has_base_type = generic_param.base_type != slake::TypeId::Invalid;
		SLKC_RETURN_IF_COMP_ERROR(writer->write_bool(has_base_type));
		if (has_base_type)
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, generic_param.base_type));

		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(generic_param.interfaces.size()));
		for (auto &k : generic_param.interfaces) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, k));
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::dump_id_ref_entries(
	peff::Alloc *allocator,
	Writer *writer,
	const peff::DynArray<slake::IdRefEntry> &entries) {
	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)entries.size()));
	for (auto &i : entries) {
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(i.name.size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->write(i.name.data(), i.name.size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(i.generic_args.size()));
		for (auto &j : i.generic_args) {
			SLKC_RETURN_IF_COMP_ERROR(dump_value(allocator, writer, j));
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::dump_id_ref(
	peff::Alloc *allocator,
	Writer *writer,
	slake::IdRefObject *ref) {
	SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, ref->entries));

	if (!ref->param_types.has_value()) {
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(UINT32_MAX));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)ref->param_types->size()));
		for (auto &i : *ref->param_types) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, i));
		}
	}
	SLKC_RETURN_IF_COMP_ERROR(writer->write_bool(ref->has_var_args));
	SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, ref->overriden_type));
	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::dump_value(
	peff::Alloc *allocator,
	Writer *writer,
	const slake::Value &value) {
	switch (value.value_type) {
		case slake::ValueType::I8:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::I8));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_i8(value.get_i8()));
			break;
		case slake::ValueType::I16:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::I16));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_i16(value.get_i16()));
			break;
		case slake::ValueType::I32:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::I32));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_i32(value.get_i32()));
			break;
		case slake::ValueType::I64:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::I64));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_i64(value.get_i64()));
			break;
		case slake::ValueType::U8:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::U8));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(value.get_u8()));
			break;
		case slake::ValueType::U16:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::U16));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u16(value.get_u16()));
			break;
		case slake::ValueType::U32:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::U32));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(value.get_u32()));
			break;
		case slake::ValueType::U64:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::U64));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u64(value.get_u64()));
			break;
		case slake::ValueType::F32:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::F32));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_f32(value.get_f32()));
			break;
		case slake::ValueType::F64:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::F64));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_f64(value.get_f64()));
			break;
		case slake::ValueType::Bool:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::Bool));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_bool(value.get_bool()));
			break;
		case slake::ValueType::TypeName:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::TypeName));
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, value.get_type_name()));
			break;
		case slake::ValueType::RegIndex:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::Reg));
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(value.get_reg_index()));
			break;
		case slake::ValueType::Reference: {
			const slake::Reference &er = value.get_reference();

			switch (er.kind) {
				case slake::ReferenceKind::ObjectRef: {
					if (!er.as_object) {
						SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::None));
						break;
					}
					switch (er.as_object->get_object_kind()) {
						case slake::ObjectKind::String: {
							slake::StringObject *s = (slake::StringObject *)er.as_object;
							SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::String));
							SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(s->data.size()));
							if (s->data.size()) {
								SLKC_RETURN_IF_COMP_ERROR(writer->write(s->data.data(), s->data.size()));
							}
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::IdRef));
							SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)er.as_object));
							break;
						}
						case slake::ObjectKind::Array: {
							slake::ArrayObject *a = (slake::ArrayObject *)er.as_object;
							SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::ValueType::Array));
							SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, a->element_type));
							SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(a->length));
							if (a->length) {
								SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)a->data, a->element_size * a->length));
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

SLKC_API peff::Option<CompilationError> slkc::dump_type_name(
	peff::Alloc *allocator,
	Writer *writer,
	const slake::TypeRef &type) {
	switch (type.type_id) {
		case slake::TypeId::Void:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Void));
			break;
		case slake::TypeId::Any:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Any));
			break;
		case slake::TypeId::I8:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::I8));
			break;
		case slake::TypeId::I16:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::I16));
			break;
		case slake::TypeId::I32:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::I32));
			break;
		case slake::TypeId::I64:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::I64));
			break;
		case slake::TypeId::U8:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::U8));
			break;
		case slake::TypeId::U16:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::U16));
			break;
		case slake::TypeId::U32:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::U32));
			break;
		case slake::TypeId::U64:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::U64));
			break;
		case slake::TypeId::F32:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::F32));
			break;
		case slake::TypeId::F64:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::F64));
			break;
		case slake::TypeId::String:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::String));
			break;
		case slake::TypeId::Bool:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Bool));
			break;
		case slake::TypeId::Array:
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Array));
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, type.get_array_type_def()->element_type->type_ref));
			break;
		case slake::TypeId::Instance: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Object));
			slake::Object *dest = type.get_custom_type_def()->type_object;
			switch (dest->get_object_kind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associated_runtime->get_full_ref(allocator, (slake::MemberObject *)dest, entries)) {
						return gen_oom_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::StructInstance: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Struct));
			slake::Object *dest = type.get_custom_type_def()->type_object;
			switch (dest->get_object_kind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::Struct: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associated_runtime->get_full_ref(allocator, (slake::MemberObject *)dest, entries)) {
						return gen_oom_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::ScopedEnum: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::ScopedEnum));
			slake::Object *dest = type.get_custom_type_def()->type_object;
			switch (dest->get_object_kind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::ScopedEnum: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associated_runtime->get_full_ref(allocator, (slake::MemberObject *)dest, entries)) {
						return gen_oom_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::TypelessScopedEnum: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::TypelessScopedEnum));
			slake::Object *dest = type.get_custom_type_def()->type_object;
			switch (dest->get_object_kind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::ScopedEnum: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associated_runtime->get_full_ref(allocator, (slake::MemberObject *)dest, entries)) {
						return gen_oom_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::UnionEnum: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::UnionEnum));
			slake::Object *dest = type.get_custom_type_def()->type_object;
			switch (dest->get_object_kind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::Struct: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associated_runtime->get_full_ref(allocator, (slake::MemberObject *)dest, entries)) {
						return gen_oom_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::UnionEnumItem: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::UnionEnumItem));
			slake::Object *dest = type.get_custom_type_def()->type_object;
			switch (dest->get_object_kind()) {
				case slake::ObjectKind::IdRef: {
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, (slake::IdRefObject *)dest));
					break;
				}
				case slake::ObjectKind::Struct: {
					peff::DynArray<slake::IdRefEntry> entries(allocator);
					if (!dest->associated_runtime->get_full_ref(allocator, (slake::MemberObject *)dest, entries)) {
						return gen_oom_comp_error();
					}
					SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, entries));
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case slake::TypeId::GenericArg: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::GenericArg));
			slake::StringObject *dest = type.get_generic_arg_type_def()->name_object;
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(dest->data.size()));
			SLKC_RETURN_IF_COMP_ERROR(writer->write(dest->data.data(), dest->data.size()));
			break;
		}
		case slake::TypeId::Ref: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Ref));
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, type.get_ref_type_def()->referenced_type->type_ref));
			break;
		}
		case slake::TypeId::ParamTypeList: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::ParamTypeList));

			slake::ParamTypeListTypeDefObject *type_def = type.get_param_type_list_type_def();

			SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)type_def->param_types.size()));

			for (size_t i = 0; i < type_def->param_types.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, type_def->param_types.at(i)->type_ref));
			}

			SLKC_RETURN_IF_COMP_ERROR(writer->write_bool(type_def->has_var_arg));

			break;
		}
		case slake::TypeId::Unpacking: {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)slake::slxfmt::TypeId::Unpacking));
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, type.get_unpacking_type_def()->type->type_ref));
			break;
		}
		default:
			std::terminate();
	}

	uint8_t type_modifier = 0;

	if (type.type_modifier & slake::TYPE_FINAL) {
		type_modifier |= slake::slxfmt::TYPE_FINAL;
	}
	if (type.type_modifier & slake::TYPE_LOCAL) {
		type_modifier |= slake::slxfmt::TYPE_LOCAL;
	}
	if (type.type_modifier & slake::TYPE_NULLABLE) {
		type_modifier |= slake::slxfmt::TYPE_NULLABLE;
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(type_modifier));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::dump_module_members(
	peff::Alloc *allocator,
	Writer *writer,
	slake::BasicModuleObject *mod) {
	peff::DynArray<slake::ClassObject *> collected_classes(allocator);
	peff::DynArray<slake::InterfaceObject *> collected_interfaces(allocator);
	peff::DynArray<slake::FnObject *> collected_fns(allocator);
	peff::DynArray<slake::StructObject *> collected_structs(allocator);
	peff::DynArray<slake::ScopedEnumObject *> collected_scoped_enums(allocator);
	peff::DynArray<slake::UnionEnumObject *> collected_union_enums(allocator);

	for (auto [k, v] : mod->get_members()) {
		switch (v->get_object_kind()) {
			case slake::ObjectKind::Class: {
				if (!collected_classes.push_back((slake::ClassObject *)v)) {
					return gen_oom_comp_error();
				}
				break;
			}
			case slake::ObjectKind::Interface: {
				if (!collected_interfaces.push_back((slake::InterfaceObject *)v)) {
					return gen_oom_comp_error();
				}
				break;
			}
			case slake::ObjectKind::Struct: {
				if (!collected_structs.push_back((slake::StructObject *)v)) {
					return gen_oom_comp_error();
				}
				break;
			}
			case slake::ObjectKind::ScopedEnum: {
				if (!collected_scoped_enums.push_back((slake::ScopedEnumObject *)v)) {
					return gen_oom_comp_error();
				}
				break;
			}
			case slake::ObjectKind::UnionEnum: {
				if (!collected_union_enums.push_back((slake::UnionEnumObject *)v)) {
					return gen_oom_comp_error();
				}
				break;
			}
			case slake::ObjectKind::Fn: {
				if (!collected_fns.push_back((slake::FnObject *)v)) {
					return gen_oom_comp_error();
				}
				break;
			}
			default:
				break;
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(collected_classes.size()));
	for (auto i : collected_classes) {
		slake::slxfmt::ClassTypeDesc desc = {};

		if (i->base_type) {
			desc.flags |= slake::slxfmt::CTD_DERIVED;
		}
		desc.num_generic_params = i->generic_params.size();
		desc.len_name = i->get_name().size();
		desc.num_impls = i->impl_types.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->get_name().data(), i->get_name().size()));

		slake::AccessModifier access = i->get_access();
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(access));

		for (auto &j : i->generic_params) {
			SLKC_RETURN_IF_COMP_ERROR(dump_generic_param(allocator, writer, j));
		}

		if (i->base_type) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, i->base_type));
		}
		for (auto &j : i->impl_types) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dump_module_members(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(collected_interfaces.size()));
	for (auto i : collected_interfaces) {
		slake::slxfmt::InterfaceTypeDesc desc = {};

		desc.num_generic_params = i->generic_params.size();
		desc.len_name = i->get_name().size();
		desc.num_parents = i->impl_types.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->get_name().data(), i->get_name().size()));

		slake::AccessModifier access_modifier = i->get_access();
		assert(slake::is_valid_access_modifier(access_modifier));
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(access_modifier));

		for (auto &j : i->generic_params) {
			SLKC_RETURN_IF_COMP_ERROR(dump_generic_param(allocator, writer, j));
		}

		for (auto &j : i->impl_types) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dump_module_members(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(collected_structs.size()));
	for (auto i : collected_structs) {
		slake::slxfmt::StructTypeDesc desc = {};

		desc.num_generic_params = i->generic_params.size();
		desc.len_name = i->get_name().size();
		desc.num_impls = i->impl_types.size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->get_name().data(), i->get_name().size()));

		slake::AccessModifier access_modifier = i->get_access();
		assert(slake::is_valid_access_modifier(access_modifier));
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(access_modifier));

		for (auto &j : i->generic_params) {
			SLKC_RETURN_IF_COMP_ERROR(dump_generic_param(allocator, writer, j));
		}

		for (auto &j : i->impl_types) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, j));
		}

		SLKC_RETURN_IF_COMP_ERROR(dump_module_members(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(collected_scoped_enums.size()));
	for (auto i : collected_scoped_enums) {
		slake::slxfmt::ScopedEnumTypeDesc desc = {};

		if (i->base_type != slake::TypeId::Invalid)
			desc.flags |= slake::slxfmt::SETD_BASE;
		desc.len_name = i->get_name().size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->get_name().data(), i->get_name().size()));

		slake::AccessModifier access_modifier = i->get_access();
		assert(slake::is_valid_access_modifier(access_modifier));
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(access_modifier));

		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(i->get_number_of_fields()));

		if (i->base_type != slake::TypeId::Invalid) {
			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, i->base_type));

			size_t j = 0;
			for (auto &record : i->get_field_records()) {
				slake::slxfmt::EnumItemDesc eid = {};
				eid.len_name = (uint32_t)record.name.size();
				SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&eid, sizeof(eid)));
				SLKC_RETURN_IF_COMP_ERROR(writer->write(record.name.data(), record.name.size()));

				slake::Value data;
				slake::Runtime::read_var(slake::StaticFieldRef(i, j), data);
				SLKC_RETURN_IF_COMP_ERROR(dump_value(allocator, writer, data));
				++j;
			}
		} else {
			size_t j = 0;
			for (auto &record : i->get_field_records()) {
				slake::slxfmt::EnumItemDesc eid = {};
				eid.len_name = (uint32_t)record.name.size();
				SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&eid, sizeof(eid)));
				SLKC_RETURN_IF_COMP_ERROR(writer->write(record.name.data(), record.name.size()));
				++j;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(collected_union_enums.size()));
	for (auto i : collected_union_enums) {
		slake::slxfmt::UnionEnumTypeDesc desc = {};

		desc.num_generic_params = i->generic_params.size();
		desc.len_name = i->get_name().size();

		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&desc, sizeof(desc)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->get_name().data(), i->get_name().size()));

		slake::AccessModifier access_modifier = i->get_access();
		assert(slake::is_valid_access_modifier(access_modifier));
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(access_modifier));

		for (auto &j : i->generic_params) {
			SLKC_RETURN_IF_COMP_ERROR(dump_generic_param(allocator, writer, j));
		}

		assert(i->get_members().size() <= UINT32_MAX);
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)i->get_members().size()));

		for (auto j : i->get_members()) {
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)j.first.size()));
			SLKC_RETURN_IF_COMP_ERROR(writer->write(j.first.data(), j.first.size()));

			assert(j.second->get_object_kind() == slake::ObjectKind::UnionEnumItem);

			slake::UnionEnumItemObject *uei = (slake::UnionEnumItemObject *)j.second;
			SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)uei->get_field_records().size()));
			for (const auto &k : uei->get_field_records()) {
				SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)k.name.size()));
				SLKC_RETURN_IF_COMP_ERROR(writer->write(k.name.data(), k.name.size()));

				SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, k.type));
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(collected_fns.size()));
	for (auto i : collected_fns) {
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(i->get_name().size()));
		SLKC_RETURN_IF_COMP_ERROR(writer->write(i->get_name().data(), i->get_name().size()));

		assert(i->overloadings.size() <= UINT32_MAX);
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(i->overloadings.size()));
		for (auto j : i->overloadings) {
			if (j.second->overloading_kind != slake::FnOverloadingKind::Regular) {
				// stub
				continue;
			}

			slake::RegularFnOverloadingObject *ol = (slake::RegularFnOverloadingObject *)j.second;

			slake::slxfmt::FnDesc fnd = {};

			if (ol->overloading_flags & slake::OL_VARG) {
				fnd.flags |= slake::slxfmt::FND_VARG;
			}
			if (ol->overloading_flags & slake::OL_GENERATOR) {
				fnd.flags |= slake::slxfmt::FND_GENERATOR;
			}
			if (ol->overloading_flags & slake::OL_VIRTUAL) {
				fnd.flags |= slake::slxfmt::FND_VIRTUAL;
			}
			fnd.num_params = ol->param_types.size();
			fnd.num_generic_params = ol->generic_params.size();
			fnd.num_registers = ol->num_registers;
			fnd.len_body = ol->instructions.size();

			SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&fnd, sizeof(fnd)));

			SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(ol->access));

			for (auto &k : j.second->generic_params) {
				SLKC_RETURN_IF_COMP_ERROR(dump_generic_param(allocator, writer, k));
			}

			for (auto &k : ol->param_types) {
				SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, k));
			}

			SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, ol->return_type));

			for (auto &k : ol->instructions) {
				SLKC_RETURN_IF_COMP_ERROR(writer->write_u8((uint8_t)k.opcode));
				SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)k.output));
				SLKC_RETURN_IF_COMP_ERROR(writer->write_u32((uint32_t)k.num_operands));
				for (size_t l = 0; l < k.num_operands; ++l) {
					SLKC_RETURN_IF_COMP_ERROR(dump_value(allocator, writer, k.operands[l]));
				}
			}
		}
	}

	const size_t num_fields = mod->get_number_of_fields();
	SLKC_RETURN_IF_COMP_ERROR(writer->write_u32(num_fields));
	for (size_t i = 0; i < num_fields; ++i) {
		const slake::FieldRecord &cur_record = mod->get_field_records().at(i);

		slake::slxfmt::VarDesc vad = {};

		vad.len_name = cur_record.name.size();
		SLKC_RETURN_IF_COMP_ERROR(writer->write((char *)&vad, sizeof(vad)));

		SLKC_RETURN_IF_COMP_ERROR(writer->write(cur_record.name.data(), cur_record.name.size()));

		assert(slake::is_valid_access_modifier(cur_record.access_modifier));
		SLKC_RETURN_IF_COMP_ERROR(writer->write_u8(cur_record.access_modifier));

		SLKC_RETURN_IF_COMP_ERROR(dump_type_name(allocator, writer, cur_record.type));
		switch (cur_record.type.type_id) {
			case slake::TypeId::Any:
			case slake::TypeId::I8:
			case slake::TypeId::I16:
			case slake::TypeId::I32:
			case slake::TypeId::I64:
			case slake::TypeId::ISize:
			case slake::TypeId::U8:
			case slake::TypeId::U16:
			case slake::TypeId::U32:
			case slake::TypeId::U64:
			case slake::TypeId::USize:
			case slake::TypeId::F32:
			case slake::TypeId::F64:
			case slake::TypeId::Bool:
			case slake::TypeId::String:
			case slake::TypeId::Instance:
			case slake::TypeId::Array:
			case slake::TypeId::Tuple:
			case slake::TypeId::SIMD:
			case slake::TypeId::Fn: {
				slake::Value data;
				slake::Runtime::read_var(slake::StaticFieldRef(mod, i), data);
				SLKC_RETURN_IF_COMP_ERROR(dump_value(allocator, writer, data));
				break;
			}
			case slake::TypeId::StructInstance:
			case slake::TypeId::UnionEnum:
			case slake::TypeId::UnionEnumItem:
			case slake::TypeId::ScopedEnum:
				break;
			case slake::TypeId::Ref:
			case slake::TypeId::TempRef:
			case slake::TypeId::GenericArg:
				break;
			case slake::TypeId::ParamTypeList:
			case slake::TypeId::Unpacking:
			case slake::TypeId::Unknown:
			default:
				std::terminate();
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::dump_module(
	peff::Alloc *allocator,
	Writer *writer,
	slake::ModuleObject *mod) {
	slake::slxfmt::ImgHeader ih = {};

	memcpy(ih.magic, slake::slxfmt::IMH_MAGIC, sizeof(ih.magic));

	ih.fmt_ver = 0x02;
	ih.num_imports = mod->unnamed_imports.size();

	SLKC_RETURN_IF_COMP_ERROR(writer->write((const char *)&ih, sizeof(ih)));

	peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

	if (!mod->associated_runtime->get_full_ref(allocator, mod, module_full_name))
		return gen_oom_comp_error();

	SLKC_RETURN_IF_COMP_ERROR(dump_id_ref_entries(allocator, writer, module_full_name));

	for (auto i : mod->unnamed_imports) {
		SLKC_RETURN_IF_COMP_ERROR(dump_id_ref(allocator, writer, i));
	}

	SLKC_RETURN_IF_COMP_ERROR(dump_module_members(allocator, writer, mod));

	return {};
}
