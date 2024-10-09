#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

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

		ModuleObject(Runtime *rt, AccessModifier access);
		ModuleObject(const ModuleObject &x);
		virtual ~ModuleObject();

		virtual ObjectKind getKind() const override;

		virtual Object *duplicate() const override;

		virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		virtual const char *getName() const override;
		virtual void setName(const char *name) override;
		virtual Object *getParent() const override;
		virtual void setParent(Object *parent) override;

		static HostObjectRef<ModuleObject> alloc(Runtime *rt, AccessModifier access);
		static HostObjectRef<ModuleObject> alloc(const ModuleObject *other);
		virtual void dealloc() override;
	};
}

#endif
