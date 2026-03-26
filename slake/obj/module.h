#ifndef _SLAKE_OBJ_MODULE_H_
#define _SLAKE_OBJ_MODULE_H_

#include "var.h"
#include <unordered_map>
#include <map>

namespace slake {
	// TODO: Flatten the field records and move the metadata onto the local field storage area.
	struct FieldRecord {
		peff::String name;
		AccessModifier access_modifier;
		size_t offset;
		TypeRef type;

		SLAKE_FORCEINLINE FieldRecord(peff::Alloc *allocator) noexcept : name(allocator), access_modifier(0), offset(SIZE_MAX), type(TypeId::Invalid) {}
		SLAKE_FORCEINLINE FieldRecord(FieldRecord &&rhs) noexcept
			: name(std::move(rhs.name)),
			  access_modifier(rhs.access_modifier),
			  offset(rhs.offset),
			  type(rhs.type) {
		}

		SLAKE_FORCEINLINE bool copy(FieldRecord &dest) const noexcept {
			if (!dest.name.build(name)) {
				return false;
			}
			dest.access_modifier = access_modifier;
			dest.offset = offset;
			dest.type = type;
			return true;
		}

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
	};

	class ModuleObject;

	using ModuleFlags = uint32_t;

	class BasicModuleObject : public MemberObject {
	protected:
		peff::HashMap<std::string_view, MemberObject *> members;

		peff::DynArray<char> local_field_storage;
		peff::DynArray<FieldRecord> field_records;
		peff::HashMap<std::string_view, size_t> field_record_indices;

		friend class Runtime;

	public:
		using MembersMap = peff::HashMap<std::string_view, MemberObject *>;
		ModuleFlags module_flags = 0;

		SLAKE_API BasicModuleObject(Runtime *rt, peff::Alloc *self_allocator, ObjectKind object_kind);
		SLAKE_API BasicModuleObject(Duplicator *duplicator, const BasicModuleObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~BasicModuleObject();

		SLAKE_API virtual Reference get_member(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool add_member(MemberObject *member);
		[[nodiscard]] SLAKE_API virtual bool shrink_member_storage();
		SLAKE_API virtual void remove_member(const std::string_view &name);

		SLAKE_API bool append_field_record(FieldRecord &&field_record);
		SLAKE_API InternalExceptionPointer append_field_record_with_value(FieldRecord &&field_record, const Value &value);
		SLAKE_API char *append_field_space(size_t size, size_t alignment);
		SLAKE_API char *append_typed_field_space(const TypeRef &type);
		SLAKE_API bool append_field_record_without_alloc(FieldRecord &&field_record);

		SLAKE_API bool realloc_field_spaces() noexcept;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;

		SLAKE_FORCEINLINE const peff::HashMap<std::string_view, MemberObject*>& get_members() const {
			return members;
		}
		SLAKE_FORCEINLINE size_t get_number_of_members() {
			return members.size();
		}
		SLAKE_FORCEINLINE size_t get_local_field_storage_size() const {
			return local_field_storage.size();
		}
		SLAKE_FORCEINLINE char* get_local_field_storage_ptr() {
			return local_field_storage.data();
		}
		SLAKE_FORCEINLINE const char *get_local_field_storage_ptr() const {
			return local_field_storage.data();
		}
		SLAKE_FORCEINLINE const FieldRecord &get_field_record(size_t index) const {
			assert(index < field_records.size());
			return field_records.at(index);
		}
		SLAKE_FORCEINLINE const peff::DynArray<FieldRecord> &get_field_records() const {
			return field_records;
		}
		SLAKE_FORCEINLINE const peff::HashMap<std::string_view, size_t> &get_field_record_indices() const {
			return field_record_indices;
		}
		SLAKE_FORCEINLINE size_t get_number_of_fields() {
			return field_records.size();
		}
		SLAKE_API peff::Option<FieldRecord &> get_field_record(const std::string_view &name);
		SLAKE_API peff::Option<const FieldRecord &> get_field_record(const std::string_view &name) const;
	};

	class ModuleObject : public BasicModuleObject {
	public:
		peff::DynArray<IdRefObject *> unnamed_imports;

		SLAKE_API ModuleObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API ModuleObject(Duplicator *duplicator, const ModuleObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(Duplicator *duplicator, const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
