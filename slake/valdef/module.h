#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
	class ModuleObject : public MemberObject {
	public:
		ModuleObject(Runtime *rt, AccessModifier access);
		inline ModuleObject(const ModuleObject &x) : MemberObject(x) {
			imports = x.imports;
			unnamedImports = x.unnamedImports;
		}
		virtual ~ModuleObject();

		std::unordered_map<std::string, IdRefObject *> imports;
		std::deque<IdRefObject *> unnamedImports;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Module; }

		virtual Object *duplicate() const override;

		static HostObjectRef<ModuleObject> alloc(Runtime *rt, AccessModifier access);
		static HostObjectRef<ModuleObject> alloc(const ModuleObject *other);
		virtual void dealloc() override;
	};
}

#endif
