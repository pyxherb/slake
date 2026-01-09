#ifndef _SLAKE_OBJ_MODULE_H_
#define _SLAKE_OBJ_MODULE_H_

#include "var.h"
#include <unordered_map>
#include <map>

namespace slake {
	// TODO: Flatten the field records and move the metadata onto the local field storage area.
	struct FieldRecord {
		peff::String name;
		AccessModifier accessModifier;
		size_t offset;
		TypeRef type;

		SLAKE_FORCEINLINE FieldRecord(peff::Alloc *allocator) : name(allocator) {}
		SLAKE_FORCEINLINE FieldRecord(FieldRecord &&rhs)
			: name(std::move(rhs.name)),
			  accessModifier(rhs.accessModifier),
			  offset(rhs.offset),
			  type(rhs.type) {
		}

		SLAKE_FORCEINLINE bool copy(FieldRecord &dest) const noexcept {
			if (!dest.name.build(name)) {
				return false;
			}
			dest.accessModifier = accessModifier;
			dest.offset = offset;
			dest.type = type;
			return true;
		}

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	class ModuleObject;

	using ModuleFlags = uint32_t;
	constexpr ModuleFlags
		_MOD_FIELDS_VALID = 0x80000000;

	class BasicModuleObject : public MemberObject {
	public:
		ModuleFlags moduleFlags = 0;

		peff::HashMap<std::string_view, MemberObject *> members;

		peff::DynArray<char> localFieldStorage;
		peff::DynArray<FieldRecord> fieldRecords;
		peff::HashMap<std::string_view, size_t> fieldRecordIndices;

		SLAKE_API BasicModuleObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind);
		SLAKE_API BasicModuleObject(Duplicator *duplicator, const BasicModuleObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~BasicModuleObject();

		SLAKE_API virtual Reference getMember(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool addMember(MemberObject *member);
		[[nodiscard]] SLAKE_API virtual bool removeMember(const std::string_view &name);

		SLAKE_API bool appendFieldRecord(FieldRecord &&fieldRecord);
		SLAKE_API char *appendFieldSpace(size_t size, size_t alignment);
		SLAKE_API char *appendTypedFieldSpace(const TypeRef &type);
		SLAKE_API bool appendFieldRecordWithoutAlloc(FieldRecord &&fieldRecord);

		SLAKE_API bool reallocFieldSpaces() noexcept;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;

	private:
		SLAKE_FORCEINLINE void _checkFieldsValidity() const noexcept {
			if (moduleFlags & _MOD_FIELDS_VALID)
				// Cannot use appendFieldRecord again until the field records are resorted.
				// The user should use `appendFieldRecordWithoutAlloc` instead.
				std::terminate();
		}
	};

	class ModuleObject : public BasicModuleObject {
	public:
		peff::DynArray<IdRefObject *> unnamedImports;

		SLAKE_API ModuleObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API ModuleObject(Duplicator *duplicator, const ModuleObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(Duplicator *duplicator, const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
