#ifndef _SLAKE_OBJ_REF_H_
#define _SLAKE_OBJ_REF_H_

#include "object.h"
#include "generic.h"
#include <utility>
#include <optional>

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

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	class IdRefObject final : public Object {
	public:
		peff::DynArray<IdRefEntry> entries;
		std::optional<peff::DynArray<Type>> paramTypes;
		bool hasVarArgs;

		SLAKE_API IdRefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API IdRefObject(const IdRefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~IdRefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

namespace std {
	SLAKE_API string to_string(std::vector<slake::IdRefEntry> &idRefEntries);
	SLAKE_API string to_string(const slake::IdRefObject *ref);
}

#endif
