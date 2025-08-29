#ifndef _SLAKE_OBJ_MEMBER_H_
#define _SLAKE_OBJ_MEMBER_H_

#include <slake/access.h>
#include <cassert>
#include <unordered_map>
#include <memory>

#include "object.h"
#include "generic.h"

namespace slake {
	class MemberObject : public Object {
	public:
		peff::String name;
		Object *parent = nullptr;

		AccessModifier accessModifier = 0;

		SLAKE_API MemberObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind);
		SLAKE_API MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~MemberObject();

		SLAKE_FORCEINLINE void setParent(Object *parent) noexcept {
			this->parent = parent;
		}
		SLAKE_API virtual const GenericArgList *getGenericArgs() const;

		SLAKE_FORCEINLINE bool setName(const std::string_view &name) noexcept {
			if (!this->name.build(name)) {
				return false;
			}
			return true;
		}

		SLAKE_FORCEINLINE void setAccess(AccessModifier accessModifier) noexcept {
			this->accessModifier = accessModifier;
		}

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
