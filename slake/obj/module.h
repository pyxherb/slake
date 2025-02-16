#ifndef _SLAKE_OBJ_MODULE_H_
#define _SLAKE_OBJ_MODULE_H_

#include "var.h"
#include <unordered_map>
#include <map>

namespace slake {
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
		std::atomic_size_t depCount = 0;

		Object *parent = nullptr;
		Scope *scope = nullptr;

		char *localFieldStorage = nullptr;
		size_t szLocalFieldStorage = 0;
		peff::DynArray<FieldRecord> fieldRecords;
		peff::HashMap<std::string_view, size_t> fieldRecordIndices;

		peff::HashMap<peff::String, IdRefObject *> imports;
		peff::DynArray<IdRefObject *> unnamedImports;

		SLAKE_API ModuleObject(Runtime *rt, ScopeUniquePtr &&scope, AccessModifier access);
		SLAKE_API ModuleObject(const ModuleObject &x, bool &succeededOut);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual ObjectRef getMember(const std::string_view &name) const override;

		SLAKE_API virtual Object *getParent() const override;
		SLAKE_API virtual void setParent(Object *parent) override;

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt, ScopeUniquePtr &&scope, AccessModifier access);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
