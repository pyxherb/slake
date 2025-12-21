#ifndef _SLAKE_OBJ_REF_H_
#define _SLAKE_OBJ_REF_H_

#include "object.h"
#include "generic.h"
#include <utility>

namespace slake {
	struct IdRefEntry final {
		peff::String name;
		ParamTypeList genericArgs;

		SLAKE_API IdRefEntry(peff::Alloc *selfAllocator);
		SLAKE_API IdRefEntry(peff::String &&name,
			ParamTypeList &&genericArgs);
		SLAKE_FORCEINLINE IdRefEntry(IdRefEntry &&rhs)
			: name(std::move(rhs.name)), genericArgs(std::move(rhs.genericArgs)) {
		}

		SLAKE_FORCEINLINE bool copy(IdRefEntry &dest) const {
			peff::constructAt<IdRefEntry>(&dest, genericArgs.allocator());

			if (!dest.name.build(name)) {
				return false;
			}

			if (!dest.genericArgs.resizeUninitialized(genericArgs.size())) {
				return false;
			}
			memcpy(dest.genericArgs.data(), genericArgs.data(), genericArgs.size() * sizeof(TypeRef));

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
		peff::Option<peff::DynArray<TypeRef>> paramTypes;
		bool hasVarArgs;
		TypeRef overridenType;

		SLAKE_API IdRefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API IdRefObject(const IdRefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~IdRefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	struct IdRefComparator {
		SLAKE_API int operator()(const IdRefObject *lhs, const IdRefObject *rhs) const noexcept;
	};

	struct IdRefLtComparator {
		IdRefComparator innerComparator;

		SLAKE_FORCEINLINE bool operator()(const IdRefObject* lhs, const IdRefObject* rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};
}

namespace std {
	SLAKE_API string to_string(std::vector<slake::IdRefEntry> &idRefEntries);
	SLAKE_API string to_string(const slake::IdRefObject *ref);
}

#endif
