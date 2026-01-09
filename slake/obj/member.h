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
	private:
		peff::String _name;
		AccessModifier _accessModifier = 0;
		Object *_parent = nullptr;

	public:
		SLAKE_API MemberObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind);
		SLAKE_API MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~MemberObject();

		SLAKE_FORCEINLINE void setParent(Object *parent) noexcept {
			_parent = parent;
		}
		SLAKE_FORCEINLINE Object *getParent() const noexcept {
			return _parent;
		}
		SLAKE_API virtual const peff::DynArray<Value> *getGenericArgs() const;

		SLAKE_API std::string_view getName() const noexcept;
		SLAKE_API bool setName(const std::string_view &name) noexcept;
		SLAKE_API bool resizeName(size_t size) noexcept;
		SLAKE_API char *getNameRawPtr() noexcept;

		SLAKE_API AccessModifier getAccess() const noexcept;
		SLAKE_API void setAccess(AccessModifier accessModifier) noexcept;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
