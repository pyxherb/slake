#ifndef _SLAKE_VALDEF_INSTANCE_H_
#define _SLAKE_VALDEF_INSTANCE_H_

#include <unordered_map>
#include <deque>

#include "member.h"
#include "generic.h"

namespace slake {
	using InstanceFlags = uint32_t;

	constexpr static InstanceFlags
		INSTANCE_PARENT = 0x01;

	class InstanceObject final : public Object {
	protected:
		GenericArgList _genericArgs;
		ClassObject *_class;

		friend class Runtime;

	public:
		InstanceObject *_parent;

		ObjectFlags instanceFlags = 0;

		inline InstanceObject(Runtime *rt, ClassObject *cls, InstanceObject *parent = nullptr)
			: Object(rt), _class(cls), _parent(parent) {
			if (parent)
				parent->instanceFlags |= INSTANCE_PARENT;
			scope = new Scope(this, parent ? parent->scope : nullptr);
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
		}

		/// @brief Delete the object and execute its destructor (if exists).
		///
		/// @note Never delete objects directly.
		virtual inline ~InstanceObject() {
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
		}

		virtual inline Type getType() const override { return Type(TypeId::Instance, (Object *)_class); }

		virtual Object *duplicate() const override;

		InstanceObject(InstanceObject &) = delete;
		InstanceObject(InstanceObject &&) = delete;
		inline InstanceObject &operator=(const InstanceObject &x) {
			(Object &)*this = (const Object &)x;

			_genericArgs = x._genericArgs;
			_class = x._class;
			instanceFlags = x.instanceFlags & ~INSTANCE_PARENT;

			return *this;
		}
		InstanceObject &operator=(InstanceObject &&) = delete;
	};
}

#endif
