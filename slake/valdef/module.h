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

		ModuleObject(Runtime *rt, AccessModifier access);
		inline ModuleObject(const ModuleObject &x) : MemberObject(x) {
			imports = x.imports;
			unnamedImports = x.unnamedImports;
			name = x.name;
			parent = x.parent;
			scope = x.scope->duplicate();
		}
		virtual ~ModuleObject();

		std::unordered_map<std::string, IdRefObject *> imports;
		std::vector<IdRefObject *> unnamedImports;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Module; }

		virtual Object *duplicate() const override;

		virtual MemberObject *getMember(
			const std::string &name,
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
