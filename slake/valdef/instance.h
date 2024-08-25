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
	public:
		GenericArgList _genericArgs;
		ClassObject *_class;

		inline InstanceObject(Runtime *rt, ClassObject *cls, InstanceObject *parent)
			: Object(rt), _class(cls), _parent(parent) {
			if (parent)
				parent->instanceFlags |= INSTANCE_PARENT;
			scope = new Scope(this, parent ? parent->scope : nullptr);
		}
		inline InstanceObject(const InstanceObject& x) : Object(x) {
			_genericArgs = x._genericArgs;
			_class = x._class;
			instanceFlags = x.instanceFlags & ~INSTANCE_PARENT;
		}
		virtual inline ~InstanceObject() {
		}

		InstanceObject *_parent;

		ObjectFlags instanceFlags = 0;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Instance; }

		virtual Object *duplicate() const override;

		static HostObjectRef<InstanceObject> alloc(Runtime *rt, ClassObject *cls, InstanceObject *parent = nullptr);
		static HostObjectRef<InstanceObject> alloc(const InstanceObject *other);
		virtual void dealloc() override;
	};
}

#endif
