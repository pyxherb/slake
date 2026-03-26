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
		AccessModifier _access_modifier = 0;
		Object *_parent = nullptr;

	public:
		SLAKE_API MemberObject(Runtime *rt, peff::Alloc *self_allocator, ObjectKind object_kind);
		SLAKE_API MemberObject(const MemberObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~MemberObject();

		SLAKE_FORCEINLINE void set_parent(Object *parent) noexcept {
			_parent = parent;
		}
		SLAKE_FORCEINLINE Object *get_parent() const noexcept {
			return _parent;
		}
		SLAKE_API virtual const peff::DynArray<Value> *get_generic_args() const;

		SLAKE_API std::string_view get_name() const noexcept;
		SLAKE_API bool set_name(const std::string_view &name) noexcept;
		SLAKE_API bool resize_name(size_t size) noexcept;
		SLAKE_API char *get_name_raw_ptr() noexcept;

		SLAKE_API AccessModifier get_access() const noexcept;
		SLAKE_API void set_access(AccessModifier access_modifier) noexcept;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
