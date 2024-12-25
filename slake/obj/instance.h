#ifndef _SLAKE_OBJ_INSTANCE_H_
#define _SLAKE_OBJ_INSTANCE_H_

#include <unordered_map>
#include <deque>

#include "var.h"
#include "generic.h"

namespace slake {
	using InstanceFlags = uint32_t;

	class InstanceObject;
	struct ObjectLayout;

	class InstanceMemberAccessorVarObject : public VarObject {
	public:
		InstanceObject *instanceObject;

		SLAKE_API InstanceMemberAccessorVarObject(Runtime *rt, InstanceObject *instanceObject);
		SLAKE_API virtual ~InstanceMemberAccessorVarObject();

		SLAKE_API static HostObjectRef<InstanceMemberAccessorVarObject> alloc(Runtime *rt, InstanceObject *arrayObject);
		SLAKE_API virtual void dealloc() override;
	};

	class InstanceObject final : public Object {
	public:
		ClassObject *_class = nullptr;
		ObjectLayout *objectLayout = nullptr;
		MethodTable *methodTable = nullptr;
		char *rawFieldData = nullptr;
		size_t szRawFieldData = 0;

		InstanceFlags instanceFlags = 0;

		InstanceMemberAccessorVarObject *memberAccessor;

		SLAKE_API InstanceObject(Runtime *rt);
		SLAKE_API InstanceObject(const InstanceObject &x);
		SLAKE_API virtual ~InstanceObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		SLAKE_API static HostObjectRef<InstanceObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<InstanceObject> alloc(const InstanceObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
