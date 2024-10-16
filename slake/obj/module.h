#ifndef _SLAKE_OBJ_MODULE_H_
#define _SLAKE_OBJ_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
	class ModuleObject : public MemberObject {
	public:
		std::string name;
		Object *parent = nullptr;
		Scope *scope;

		std::unordered_map<std::pmr::string, IdRefObject *> imports;
		std::vector<IdRefObject *> unnamedImports;

		SLAKE_API ModuleObject(Runtime *rt, AccessModifier access);
		SLAKE_API ModuleObject(const ModuleObject &x);
		SLAKE_API virtual ~ModuleObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		SLAKE_API virtual const char *getName() const override;
		SLAKE_API virtual void setName(const char *name) override;
		SLAKE_API virtual Object *getParent() const override;
		SLAKE_API virtual void setParent(Object *parent) override;

		SLAKE_API static HostObjectRef<ModuleObject> alloc(Runtime *rt, AccessModifier access);
		SLAKE_API static HostObjectRef<ModuleObject> alloc(const ModuleObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
