#ifndef _SLAKE_OBJ_REF_H_
#define _SLAKE_OBJ_REF_H_

#include "object.h"
#include "generic.h"
#include <utility>

namespace slake {
	struct IdRefEntry final {
		peff::String name;
		peff::DynArray<Value> generic_args;

		SLAKE_API IdRefEntry(peff::Alloc *self_allocator);
		SLAKE_API IdRefEntry(peff::String &&name,
			peff::DynArray<Value> &&generic_args);
		SLAKE_FORCEINLINE IdRefEntry(IdRefEntry &&rhs) noexcept
			: name(std::move(rhs.name)), generic_args(std::move(rhs.generic_args)) {
		}

		SLAKE_FORCEINLINE bool copy(IdRefEntry &dest) const {
			peff::construct_at<IdRefEntry>(&dest, generic_args.allocator());

			if (!dest.name.build(name)) {
				return false;
			}

			if (!dest.generic_args.build(generic_args)) {
				return false;
			}

			return true;
		}
		SLAKE_FORCEINLINE IdRefEntry &operator=(IdRefEntry &&rhs) noexcept {
			name = std::move(rhs.name);
			generic_args = std::move(rhs.generic_args);

			return *this;
		}

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
	};

	class IdRefObject final : public Object {
	public:
		peff::DynArray<IdRefEntry> entries;
		peff::Option<peff::DynArray<TypeRef>> param_types;
		bool has_var_args;
		TypeRef overriden_type;

		SLAKE_API IdRefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API IdRefObject(const IdRefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~IdRefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	struct IdRefComparator {
		SLAKE_API int operator()(const IdRefObject *lhs, const IdRefObject *rhs) const noexcept;
	};

	struct IdRefLtComparator {
		IdRefComparator inner_comparator;

		SLAKE_FORCEINLINE bool operator()(const IdRefObject *lhs, const IdRefObject *rhs) const noexcept {
			return inner_comparator(lhs, rhs) < 0;
		}
	};
}

namespace std {
	SLAKE_API string to_string(std::vector<slake::IdRefEntry> &id_ref_entries);
	SLAKE_API string to_string(const slake::IdRefObject *ref);
}

#endif
