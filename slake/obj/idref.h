#ifndef _SLAKE_OBJ_REF_H_
#define _SLAKE_OBJ_REF_H_

#include "object.h"
#include "generic.h"
#include <utility>

namespace slake {
	struct IdRefEntry final {
		peff::String name;
		GenericArgList genericArgs;

		SLAKE_API IdRefEntry(peff::Alloc *selfAllocator);
		SLAKE_API IdRefEntry(peff::String &&name,
			GenericArgList &&genericArgs);
		SLAKE_FORCEINLINE IdRefEntry(IdRefEntry &&rhs)
			: name(std::move(rhs.name)), genericArgs(std::move(rhs.genericArgs)) {
		}

		SLAKE_FORCEINLINE bool copy(IdRefEntry &dest) const {
			peff::constructAt<IdRefEntry>(&dest, genericArgs.allocator());

			if (!peff::copyAssign(dest.name, name)) {
				return false;
			}
			if (!peff::copyAssign(dest.genericArgs, genericArgs)) {
				return false;
			}

			return true;
		}
		SLAKE_FORCEINLINE IdRefEntry &operator=(IdRefEntry &&rhs) noexcept {
			name = std::move(rhs.name);
			genericArgs = std::move(rhs.genericArgs);

			return *this;
		}
	};

	class IdRefObject final : public Object {
	public:
		peff::DynArray<IdRefEntry> entries;
		peff::DynArray<Type> paramTypes;
		bool hasVarArgs;

		SLAKE_API IdRefObject(Runtime *rt);
		SLAKE_API IdRefObject(const IdRefObject &x, bool &succeededOut);
		SLAKE_API virtual ~IdRefObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

namespace std {
	SLAKE_API string to_string(std::vector<slake::IdRefEntry> &idRefEntries);
	SLAKE_API string to_string(const slake::IdRefObject *ref);
}

#endif
