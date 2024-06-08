#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
	class ModuleObject : public MemberObject {
	public:
		ModuleObject(Runtime *rt, AccessModifier access);
		virtual ~ModuleObject();

		std::unordered_map<std::string, IdRefObject *> imports;
		std::deque<IdRefObject *> unnamedImports;

		virtual Type getType() const override;

		virtual Object *duplicate() const override;

		static HostObjectRef<ModuleObject> alloc(Runtime *rt, AccessModifier access);
		virtual void dealloc() override;

		inline ModuleObject &operator=(const ModuleObject &x) {
			((MemberObject &)*this) = (MemberObject &)x;

			imports = x.imports;
			unnamedImports = x.unnamedImports;

			return *this;
		}
		ModuleObject &operator=(ModuleObject &&) = delete;
	};
}

#endif
