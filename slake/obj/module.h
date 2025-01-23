#ifndef _SLAKE_OBJ_MODULE_H_
#define _SLAKE_OBJ_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
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

		peff::HashMap<peff::String, IdRefObject *> imports;
		peff::DynArray<IdRefObject *> unnamedImports;

		SLAKE_API ModuleObject(Runtime *rt, ScopeUniquePtr &&scope, AccessModifier access);
		SLAKE_API ModuleObject(const ModuleObject &x, bool &succeededOut);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual MemberObject *getMember(
			const std::string_view &name,
			VarRefContext *varRefContextOut) const;

		SLAKE_API virtual Object *getParent() const override;
		SLAKE_API virtual void setParent(Object *parent) override;

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt, ScopeUniquePtr &&scope, AccessModifier access);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
