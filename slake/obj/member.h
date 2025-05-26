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
	protected:
		SLAKE_API virtual bool onSetParent(Object *parent);

	public:
		peff::String name;
		Object *parent = nullptr;

		AccessModifier accessModifier = 0;

		SLAKE_API MemberObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~MemberObject();

		[[nodiscard]] SLAKE_FORCEINLINE bool setParent(Object *parent) noexcept {
			if(!onSetParent(parent))
				return false;
			this->parent = parent;
			return true;
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
