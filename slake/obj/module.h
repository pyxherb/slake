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

	class ModuleObject : public MemberObject {
	public:
		peff::HashMap<std::string_view, MemberObject *> members;

		peff::DynArray<char> localFieldStorage;
		peff::DynArray<FieldRecord> fieldRecords;
		peff::HashMap<std::string_view, size_t> fieldRecordIndices;

		peff::DynArray<IdRefObject *> unnamedImports;

		SLAKE_API ModuleObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind = ObjectKind::Module);
		SLAKE_API ModuleObject(Duplicator *duplicator, const ModuleObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API virtual Reference getMember(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool addMember(MemberObject *member);
		[[nodiscard]] SLAKE_API virtual bool removeMember(const std::string_view &name);

		SLAKE_API bool appendFieldRecord(FieldRecord &&fieldRecord);
		SLAKE_API char *appendFieldSpace(size_t size, size_t alignment);
		SLAKE_API char *appendTypedFieldSpace(const TypeRef &type);

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(Duplicator *duplicator, const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
