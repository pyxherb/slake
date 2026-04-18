#include <slake/runtime.h>
#include <slake/loader/loader.h>
#include <slake/opti/proganal.h>
// #include <slake/lib/std.h>

#include <iostream>

using namespace slake;

Value print(Context *context, MajorFrame *cur_major_frame) {
	if (cur_major_frame->resumable_context_data.num_args < 1)
		putchar('\n');
	else {
		for (uint8_t i = 0; i < cur_major_frame->resumable_context_data.num_args; ++i) {
			Value data;
			if (cur_major_frame->cur_coroutine)
				Runtime::read_var(CoroutineArgRef(cur_major_frame->cur_coroutine, i), data);
			else
				Runtime::read_var(ArgRef(cur_major_frame, i), data);

			switch (data.value_type) {
				case ValueType::I8:
					std::cout << data.get_i8();
					break;
				case ValueType::I16:
					std::cout << data.get_i16();
					break;
				case ValueType::I32:
					std::cout << data.get_i32();
					break;
				case ValueType::I64:
					std::cout << data.get_i64();
					break;
				case ValueType::U8:
					std::cout << data.get_u8();
					break;
				case ValueType::U16:
					std::cout << data.get_u16();
					break;
				case ValueType::U32:
					std::cout << data.get_u32();
					break;
				case ValueType::U64:
					std::cout << data.get_u64();
					break;
				case ValueType::F32:
					std::cout << data.get_f32();
					break;
				case ValueType::F64:
					std::cout << data.get_f64();
					break;
				case ValueType::Bool:
					fputs(data.get_bool() ? "true" : "false", stdout);
					break;
				case ValueType::Reference: {
					Object *object_ptr = data.get_reference().as_object;
					if (!object_ptr)
						fputs("null", stdout);
					else {
						switch (object_ptr->get_object_kind()) {
							case ObjectKind::String:
								std::cout << ((StringObject *)object_ptr)->data.data();
								break;
							default:
								std::cout << "<object at " << std::hex << object_ptr << ">";
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

void print_traceback(Runtime *rt, ContextObject *context) {
	printf("Traceback:\n");

	auto walker = [](MajorFrame *i, void *user_data) {
		if (!i->cur_fn) {
			printf("(Stack top)\n");
			return true;
		}

		peff::DynArray<IdRefEntry> full_ref(peff::default_allocator());

		if (!i->associated_runtime->get_full_ref(peff::default_allocator(), i->cur_fn->fn_object, full_ref)) {
			throw std::bad_alloc();
		}

		std::string name;

		for (size_t i = 0; i < full_ref.size(); ++i) {
			if (i) {
				name += '.';
			}

			IdRefEntry &id = full_ref.at(i);

			name += id.name;

			if (id.generic_args.size()) {
				name += '<';

				/* for (size_t j = 0; j < id.generic_args.size(); ++j) {
					if (j)
						name += ",";
					name += std::to_string(id.generic_args.at(j), rt);
				}*/

				name += '>';
			}
		}

		printf("\t%s: %u", name.c_str(), i->resumable_context_data.cur_ins);
		putchar('\n');

		return true;
	};

	context->get_context().for_each_major_frame(walker, nullptr);
}

class MyReader : public loader::Reader {
public:
	peff::RcObjectPtr<peff::Alloc> self_allocator;

	FILE *fp;

	MyReader(peff::Alloc *self_allocator, FILE *fp) : self_allocator(self_allocator), fp(fp) {}
	virtual ~MyReader() {
		fclose(fp);
	}

	virtual bool is_eof() noexcept override {
		return feof(fp);
	}

	virtual loader::ReadResult read(char *buffer, size_t size) noexcept override {
		if (fread(buffer, size, 1, fp) < 1) {
			return loader::ReadResult::ReadError;
		}
		return loader::ReadResult::Succeeded;
	}

	virtual void dealloc() noexcept override {
		peff::destroy_and_release<MyReader>(self_allocator.get(), this, alignof(MyReader));
	}
};

class LoaderContext : public loader::LoaderContext {
public:
	LoaderContext(peff::Alloc *allocator) : loader::LoaderContext(allocator) {
	}
	~LoaderContext() {
	}

	virtual InternalExceptionPointer locate_module(Runtime *rt, const peff::DynArray<IdRefEntry> &ref, loader::Reader *&reader_out) {
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
			return BadMagicError::alloc(rt->get_fixed_alloc());
		}

		peff::ScopeGuard close_fp_guard([fp]() noexcept {
			fclose(fp);
		});

		std::unique_ptr<MyReader, peff::DeallocableDeleter<MyReader>> reader(peff::alloc_and_construct<MyReader>(allocator.get(), alignof(MyReader), allocator.get(), fp));

		if (!reader)
			return OutOfMemoryError::alloc();

		reader_out = reader.release();

		close_fp_guard.release();

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

[[nodiscard]] SLAKE_API bool dump_type_name(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type);
[[nodiscard]] SLAKE_API bool dump_value(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value);
[[nodiscard]] SLAKE_API bool dump_id_ref_entries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &id_ref_in);
[[nodiscard]] SLAKE_API bool dump_id_ref(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<IdRefEntry> &entries, peff::DynArray<TypeRef> *optional_param_types, bool has_var_args, peff::Option<TypeRef> overriden_type);
[[nodiscard]] SLAKE_API bool dump_id_ref(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *id_ref_in);

#define SLAKE_RETURN_IF_FALSE(e) \
	if (!e) return false

SLAKE_API bool dump_value(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value) {
	switch (value.value_type) {
		case slake::ValueType::I8: {
			char s[8];
			snprintf(s, sizeof(s) - 1, "%hd", (int16_t)value.get_i8());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I16: {
			char s[16];
			snprintf(s, sizeof(s) - 1, "%hd", (int16_t)value.get_i16());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I32: {
			char s[32];
			snprintf(s, sizeof(s) - 1, "%d", value.get_i32());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::I64: {
			char s[48];
			snprintf(s, sizeof(s) - 1, "%lld", value.get_i64());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U8: {
			char s[4];
			snprintf(s, sizeof(s) - 1, "%hu", (uint16_t)value.get_u8());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U16: {
			char s[8];
			snprintf(s, sizeof(s) - 1, "%hu", (uint16_t)value.get_u16());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U32: {
			char s[16];
			snprintf(s, sizeof(s) - 1, "%u", value.get_u32());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::U64: {
			char s[32];
			snprintf(s, sizeof(s) - 1, "%llu", value.get_u64());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F32: {
			char s[16];
			snprintf(s, sizeof(s) - 1, "%f", value.get_f32());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::F64: {
			char s[32];
			snprintf(s, sizeof(s) - 1, "%f", value.get_f64());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::Bool:
			SLAKE_RETURN_IF_FALSE(writer->write(value.get_bool() ? "true" : "false"));
			break;
		case slake::ValueType::Reference: {
			const slake::Reference &er = value.get_reference();

			switch (er.kind) {
				case slake::ReferenceKind::ObjectRef: {
					slake::Object *obj = er.as_object;

					if (!obj) {
						SLAKE_RETURN_IF_FALSE(writer->write("null"));
						break;
					}

					switch (obj->get_object_kind()) {
						case slake::ObjectKind::String: {
							SLAKE_RETURN_IF_FALSE(writer->write("\""));

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
										SLAKE_RETURN_IF_FALSE(writer->write(std::string_view(data + idx_char_since_last_esc, i - idx_char_since_last_esc)));

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

										idx_char_since_last_esc = i + 1;
										break;

									default:

										break;
								}
							}

							if (idx_char_since_last_esc < len)
								SLAKE_RETURN_IF_FALSE(writer->write(std::string_view(data + idx_char_since_last_esc, len - idx_char_since_last_esc)));

							SLAKE_RETURN_IF_FALSE(writer->write("\""));
							break;
						}
						case slake::ObjectKind::IdRef: {
							SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj));
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

								slake::Runtime::read_var(rer, data);

								SLAKE_RETURN_IF_FALSE(dump_value(allocator, writer, data));
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
			snprintf(s, sizeof(s) - 1, "%%%u", value.get_reg_index());
			SLAKE_RETURN_IF_FALSE(writer->write(s));
			break;
		}
		case slake::ValueType::TypeName: {
			SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, value.get_type_name()));
			break;
		}
		default:
			std::terminate();
	}

	return true;
}

SLAKE_API bool dump_id_ref_entries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &id_ref_in) {
	for (size_t i = 0; i < id_ref_in.size(); ++i) {
		if (i) {
			SLAKE_RETURN_IF_FALSE(writer->write("."));
		}

		auto &cur_entry = id_ref_in.at(i);

		SLAKE_RETURN_IF_FALSE(writer->write(cur_entry.name));

		if (cur_entry.generic_args.size()) {
			SLAKE_RETURN_IF_FALSE(writer->write("<"));
			for (size_t j = 0; j < cur_entry.generic_args.size(); ++j) {
				if (j) {
					SLAKE_RETURN_IF_FALSE(writer->write(","));
				}
				SLAKE_RETURN_IF_FALSE(dump_value(allocator, writer, cur_entry.generic_args.at(j)));
			}
			SLAKE_RETURN_IF_FALSE(writer->write(">"));
		}
	}

	return true;
}

SLAKE_API bool dump_id_ref(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<IdRefEntry> &entries, peff::DynArray<TypeRef> *optional_param_types, bool has_var_args, peff::Option<TypeRef> overriden_type) {
	SLAKE_RETURN_IF_FALSE(dump_id_ref_entries(allocator, writer, entries));

	if (optional_param_types) {
		auto &param_types = *optional_param_types;

		if (param_types.size()) {
			SLAKE_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < param_types.size(); ++i) {
				if (i) {
					SLAKE_RETURN_IF_FALSE(writer->write(", "));
				}

				SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, param_types.at(i)));
			}

			if (has_var_args) {
				SLAKE_RETURN_IF_FALSE(writer->write(", ..."));
			}

			SLAKE_RETURN_IF_FALSE(writer->write(")"));
		} else {
			if (has_var_args) {
				SLAKE_RETURN_IF_FALSE(writer->write("(...)"));
			} else {
				SLAKE_RETURN_IF_FALSE(writer->write("()"));
			}
		}
	} else {
		if (has_var_args) {
			SLAKE_RETURN_IF_FALSE(writer->write("(...)"));
		}
	}

	if (overriden_type) {
		SLAKE_RETURN_IF_FALSE(writer->write(" override "));
		SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, *overriden_type));
	}

	return true;
}

SLAKE_API bool dump_id_ref(peff::Alloc *allocator, DumpWriter *writer, IdRefObject *id_ref_object) {
	SLAKE_RETURN_IF_FALSE(dump_id_ref(
		allocator,
		writer,
		id_ref_object->entries,
		id_ref_object->param_types ? &id_ref_object->param_types.value() : nullptr,
		id_ref_object->has_var_args,
		peff::Option<TypeRef>(TypeRef(id_ref_object->overriden_type))));

	return true;
}

SLAKE_API bool dump_type_name(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type) {
	switch (type.type_id) {
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
			auto obj = type.get_custom_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			switch (obj->type_object->get_object_kind()) {
				case slake::ObjectKind::Class:
				case slake::ObjectKind::Interface: {
					SLAKE_RETURN_IF_FALSE(writer->write("@"));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, module_full_name, nullptr, false, {}));

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("@"));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
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
					SLAKE_RETURN_IF_FALSE(writer->write("struct "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, module_full_name, nullptr, false, {}));

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("struct "));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
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
					SLAKE_RETURN_IF_FALSE(writer->write("enum base "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, module_full_name, nullptr, false, {}));

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum base "));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
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
					SLAKE_RETURN_IF_FALSE(writer->write("enum "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, module_full_name, nullptr, false, {}));

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum "));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
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
					SLAKE_RETURN_IF_FALSE(writer->write("enum union "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, module_full_name, nullptr, false, {}));

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum union "));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
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
					SLAKE_RETURN_IF_FALSE(writer->write("enum struct "));

					peff::DynArray<slake::IdRefEntry> module_full_name(allocator);

					if (!runtime->get_full_ref(allocator, (slake::MemberObject *)obj->type_object, module_full_name))
						return false;

					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, module_full_name, nullptr, false, {}));

					break;
				}
				case slake::ObjectKind::IdRef: {
					SLAKE_RETURN_IF_FALSE(writer->write("enum struct "));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, (slake::IdRefObject *)obj->type_object));
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

			SLAKE_RETURN_IF_FALSE(writer->write("@!"));
			SLAKE_RETURN_IF_FALSE(writer->write(obj->name_object->data.data(), obj->name_object->data.size()));
			break;
		}
		case slake::TypeId::Array: {
			auto obj = type.get_array_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, obj->element_type->type_ref));
			SLAKE_RETURN_IF_FALSE(writer->write("[]"));
			break;
		}
		case slake::TypeId::Ref: {
			auto obj = type.get_ref_type_def();

			slake::Runtime *runtime = obj->associated_runtime;

			SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, obj->referenced_type->type_ref));
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
			auto obj = type.get_param_type_list_type_def();

			SLAKE_RETURN_IF_FALSE(writer->write("("));

			for (size_t i = 0; i < obj->param_types.size(); ++i) {
				if (i) {
					SLAKE_RETURN_IF_FALSE(writer->write(", "));
				}

				SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, obj->param_types.at(i)->type_ref));
			}

			SLAKE_RETURN_IF_FALSE(writer->write(")"));
			break;
		}
		case slake::TypeId::Tuple: {
			auto obj = type.get_tuple_type_def();

			SLAKE_RETURN_IF_FALSE(writer->write("["));

			for (size_t i = 0; i < obj->element_types.size(); ++i) {
				if (i) {
					SLAKE_RETURN_IF_FALSE(writer->write(", "));
				}

				SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, obj->element_types.at(i)->type_ref));
			}

			SLAKE_RETURN_IF_FALSE(writer->write("]"));
			break;
		}
		case slake::TypeId::SIMD: {
			auto obj = type.get_simdtype_def();

			SLAKE_RETURN_IF_FALSE(writer->write("simd_t<"));

			SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, obj->type->type_ref));

			SLAKE_RETURN_IF_FALSE(writer->write(", "));

			char s[16];
			snprintf(s, sizeof(s) - 1, "%u", obj->width);
			SLAKE_RETURN_IF_FALSE(writer->write(s));

			SLAKE_RETURN_IF_FALSE(writer->write(">"));
			break;
		}
		case slake::TypeId::Unpacking: {
			auto obj = type.get_unpacking_type_def();
			SLAKE_RETURN_IF_FALSE(writer->write("@..."));
			SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, obj->type->type_ref));
			break;
		}
		default:
			std::terminate();
	}

	if (type.is_nullable())
		SLAKE_RETURN_IF_FALSE(writer->write("?"));
	return true;
}

bool dump_exception_info(peff::Alloc *allocator, DumpWriter *writer, slake::InternalException *e) {
	SLAKE_RETURN_IF_FALSE(writer->write(e->what()));
	SLAKE_RETURN_IF_FALSE(writer->write("\n"));
	auto write_details_header = [writer]() -> bool {
		SLAKE_RETURN_IF_FALSE(writer->write("Details:\n"));
		return true;
	};
	switch (e->kind) {
		case ErrorKind::OutOfMemoryError:
			break;
		case ErrorKind::RuntimeExecError: {
			RuntimeExecError *rte = (RuntimeExecError *)e;
			switch (rte->error_code) {
				case slake::RuntimeExecErrorCode::MismatchedVarType: {
					MismatchedVarTypeError *err = (MismatchedVarTypeError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Mismatched variable type, expecting "));
					SLAKE_RETURN_IF_FALSE(dump_type_name(allocator, writer, err->expected_type));
					break;
				}
				case slake::RuntimeExecErrorCode::ReferencedMemberNotFound: {
					ReferencedMemberNotFoundError *err = (ReferencedMemberNotFoundError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Referenced member not found: "));
					SLAKE_RETURN_IF_FALSE(dump_id_ref(allocator, writer, err->id_ref.get()));
					break;
				}
				case slake::RuntimeExecErrorCode::UncaughtException: {
					UncaughtExceptionError *err = (UncaughtExceptionError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Uncaught exception: "));
					SLAKE_RETURN_IF_FALSE(dump_value(allocator, writer, err->exception_value));
					break;
				}
				case slake::RuntimeExecErrorCode::FrameBoundaryExceeded: {
					FrameBoundaryExceededError *err = (FrameBoundaryExceededError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Frame boundary exceeded"));
					break;
				}
				case slake::RuntimeExecErrorCode::InvalidArgumentNumber: {
					InvalidArgumentNumberError *err = (InvalidArgumentNumberError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Invalid argument number, "));
					{
						char n[13];
						snprintf(n, sizeof(n) - 1, "%u", err->num_args);
						SLAKE_RETURN_IF_FALSE(writer->write(n));
					}
					SLAKE_RETURN_IF_FALSE(writer->write(" arguments does not match"));
					break;
				}
				case slake::RuntimeExecErrorCode::InvalidArrayIndex: {
					InvalidArrayIndexError *err = (InvalidArrayIndexError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Invalid array index, "));
					{
						char n[26];
						snprintf(n, sizeof(n) - 1,"%zu", err->index);
						SLAKE_RETURN_IF_FALSE(writer->write(n));
					}
					SLAKE_RETURN_IF_FALSE(writer->write(" is out of index range"));
					break;
				}
				case slake::RuntimeExecErrorCode::StackOverflow: {
					StackOverflowError *err = (StackOverflowError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
					SLAKE_RETURN_IF_FALSE(writer->write("Stack overflowed"));
					break;
				}
				case slake::RuntimeExecErrorCode::MalformedClassStructure: {
					MalformedClassStructureError *err = (MalformedClassStructureError *)rte;
					SLAKE_RETURN_IF_FALSE(write_details_header());
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
	util::setup_memory_leak_detector();

	StdDumpWriter stderr_writer(stderr);
	{
		std::unique_ptr<Runtime, peff::DeallocableDeleter<Runtime>> rt = std::unique_ptr<Runtime, peff::DeallocableDeleter<Runtime>>(
			Runtime::alloc(
				peff::default_allocator(),
				peff::default_allocator(),
				RT_DEBUG | RT_GCDBG));

		{
			HostObjectRef<ModuleObject> mod;
			{
				FILE *fp;

				if (!(fp = fopen("hostext/main.slx", "rb"))) {
					puts("Error opening the main module");
					return -1;
				}

				peff::ScopeGuard close_fp_guard([fp]() noexcept {
					fclose(fp);
				});

				LoaderContext loader_context(peff::default_allocator());
				MyReader reader(&peff::g_null_alloc, fp);

				close_fp_guard.release();

				if (auto e = loader::load_module(loader_context, rt.get(), &reader, mod); e) {
					printf("Error loading main module:\n");
					if (!dump_exception_info(peff::default_allocator(), &stderr_writer, e.get()))
						abort();
					e.reset();
					return -1;
				}
			}

			{
				HostRefHolder host_ref_holder(rt->get_fixed_alloc());

				HostObjectRef<ModuleObject> mod_object_hostext;

				mod_object_hostext = (ModuleObject *)rt->get_root_object()->get_member("hostext").get_object_ref();
				if ((!mod_object_hostext) || (mod_object_hostext->get_name() != "hostext"))
					std::terminate();

				/*
				if (!mod_object_hostext->set_name("hostext")) {
					std::terminate();
				}*/
				HostObjectRef<ModuleObject> mod_object_extfns = ModuleObject::alloc(rt.get());
				if (!mod_object_extfns)
					std::terminate();
				if (!mod_object_extfns->set_name("extfns")) {
					std::terminate();
				}

				if (!mod_object_hostext->add_member(mod_object_extfns.get())) {
					std::terminate();
				}

				mod_object_extfns = (ModuleObject *)mod_object_hostext->get_member("extfns").get_object_ref();
				if ((!mod_object_extfns) || (mod_object_extfns->get_name() != "extfns"))
					std::terminate();

				HostObjectRef<FnObject> fn_object = FnObject::alloc(rt.get());

				auto print_fn = NativeFnOverloadingObject::alloc(
					fn_object.get(),
					print);
				print_fn->set_access(slake::make_access_modifier(slake::AccessMode::Public));
				print_fn->return_type = TypeId::Void;
				print_fn->set_var_args();
				print_fn->overriden_type = TypeId::Void;
				if (!fn_object->overloadings.insert({ print_fn->param_types, print_fn->is_with_var_args(), print_fn->generic_params.size(), print_fn->overriden_type }, print_fn.get()))
					throw std::bad_alloc();
				fn_object->set_name("print");

				mod_object_extfns->remove_member("print");
				if (!mod_object_extfns->add_member(fn_object.get()))
					throw std::bad_alloc();

				auto fn = (FnObject *)mod->get_member("main").as_object;
				FnOverloadingObject *overloading;

				peff::DynArray<TypeRef> params(peff::default_allocator());

				overloading = fn->overloadings.at(FnSignature(params, false, 0, TypeId::Void));

				/* opti::ProgramAnalyzedInfo analyzed_info(rt.get(), &my_allocator);
				if (auto e = opti::analyze_program_info_pass(rt.get(), &my_allocator, (RegularFnOverloadingObject *)overloading, analyzed_info, host_ref_holder);
					e) {
					printf("Internal exception: %s\n", e->what());
					switch (e->kind) {
						case ErrorKind::OptimizerError: {
							OptimizerError *err = (OptimizerError *)e.get();

							switch (err->optimizer_error_code) {
								case OptimizerErrorCode::MalformedProgram: {
									MalformedProgramError *err = (MalformedProgramError *)e.get();

									printf("Malformed program error at instruction #%zu\n", err->off_ins);
								}
								default:;
							}
						}
						default:;
					}
					e.reset();
					goto end;
				}
				for (auto it = analyzed_info.analyzed_reg_info.begin(); it != analyzed_info.analyzed_reg_info.end(); ++it) {
					printf("Register #%u\n", it.key());
					printf("Lifetime: %u-%u\n", it.value().lifetime.off_begin_ins, it.value().lifetime.off_end_ins);
				}*/

				{
					HostObjectRef<CoroutineObject> co;
					if (auto e = rt->create_coroutine_instance(overloading, nullptr, nullptr, 0, co); e) {
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
					while (!co->is_done()) {
						if (auto e = rt->resume_coroutine(context.get(), co.get(), result);
							e) {
							printf("Internal exception: %s\n", e->what());

							if (!dump_exception_info(peff::default_allocator(), &stderr_writer, e.get()))
								abort();

							print_traceback(rt.get(), context.get());
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
