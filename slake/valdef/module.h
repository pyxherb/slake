#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
	class ModuleObject : public MemberObject {
	public:
		std::unordered_map<std::string, IdRefObject *> imports;
		std::deque<IdRefObject *> unnamedImports;

		ModuleObject(Runtime *rt, AccessModifier access);
		virtual ~ModuleObject();

		virtual Type getType() const override;

		virtual Object *duplicate() const override;

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
