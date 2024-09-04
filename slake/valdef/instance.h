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
		Scope *scope;
		MethodTable *methodTable;

		inline InstanceObject(Runtime *rt, ClassObject *cls, InstanceObject *parent)
			: Object(rt), _class(cls), _parent(parent) {
			if (parent)
				parent->instanceFlags |= INSTANCE_PARENT;
			scope = new Scope(this);
		}
		inline InstanceObject(const InstanceObject& x) : Object(x) {
			_genericArgs = x._genericArgs;
			_class = x._class;
			instanceFlags = x.instanceFlags & ~INSTANCE_PARENT;
		}
		virtual inline ~InstanceObject() {
			if (scope)
				delete scope;

			// DO NOT DELETE THE METHOD TABLE!!!
			// The method table is borrowed from the class.
		}

		InstanceObject *_parent;

		ObjectFlags instanceFlags = 0;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Instance; }

		virtual Object *duplicate() const override;

		virtual MemberObject *getMember(
			const std::string &name,
			VarRefContext *varRefContextOut) const;

		static HostObjectRef<InstanceObject> alloc(Runtime *rt, ClassObject *cls, InstanceObject *parent = nullptr);
		static HostObjectRef<InstanceObject> alloc(const InstanceObject *other);
		virtual void dealloc() override;
	};
}

#endif
