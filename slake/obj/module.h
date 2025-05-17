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
		Type type;

		SLAKE_FORCEINLINE FieldRecord(peff::Alloc *allocator) : name(allocator) {}
		SLAKE_FORCEINLINE FieldRecord(FieldRecord &&rhs)
			: name(std::move(rhs.name)),
			  accessModifier(rhs.accessModifier),
			  offset(rhs.offset),
			  type(rhs.type) {
		}

		SLAKE_FORCEINLINE bool copy(FieldRecord &dest) const noexcept {
			if (!peff::copy(dest.name, name)) {
				return false;
			}
			dest.accessModifier = accessModifier;
			dest.offset = offset;
			dest.type = type;
			return true;
		}
	};

	class ModuleObject;

	enum class ModuleLoadStatus {
		ImplicitlyLoaded = 0,
		Loading,
		ManuallyLoaded
	};

	class ModuleObject : public MemberObject {
	public:
		ModuleLoadStatus loadStatus = ModuleLoadStatus::ImplicitlyLoaded;

		peff::HashMap<std::string_view, MemberObject *> members;

		peff::DynArray<char> localFieldStorage;
		peff::DynArray<FieldRecord> fieldRecords;
		peff::HashMap<std::string_view, size_t> fieldRecordIndices;

		peff::DynArray<IdRefObject *> unnamedImports;

		SLAKE_API ModuleObject(Runtime *rt);
		SLAKE_API ModuleObject(const ModuleObject &x, bool &succeededOut);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual EntityRef getMember(const std::string_view &name) const override;
		[[nodiscard]] SLAKE_API virtual bool addMember(MemberObject *member);
		SLAKE_API virtual void removeMember(const std::string_view &name);
		[[nodiscard]] SLAKE_API virtual bool removeMemberAndTrim(const std::string_view &name);

		SLAKE_API bool appendFieldRecord(FieldRecord &&fieldRecord);
		SLAKE_API char *appendFieldSpace(size_t size, size_t alignment);
		SLAKE_API char *appendTypedFieldSpace(const Type &type);

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
