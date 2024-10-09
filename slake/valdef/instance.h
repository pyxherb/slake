#ifndef _SLAKE_VALDEF_INSTANCE_H_
#define _SLAKE_VALDEF_INSTANCE_H_

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

		InstanceMemberAccessorVarObject(Runtime *rt, InstanceObject *instanceObject);
		virtual ~InstanceMemberAccessorVarObject();

		virtual Type getVarType(const VarRefContext &context) const override;

		virtual VarKind getVarKind() const override { return VarKind::InstanceMemberAccessor; }

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<InstanceMemberAccessorVarObject> alloc(Runtime *rt, InstanceObject *arrayObject);
		virtual void dealloc() override;
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

		InstanceObject(Runtime *rt);
		InstanceObject(const InstanceObject &x);
		virtual ~InstanceObject();

		virtual ObjectKind getKind() const override;

		virtual Object *duplicate() const override;

		virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		static HostObjectRef<InstanceObject> alloc(Runtime *rt);
		static HostObjectRef<InstanceObject> alloc(const InstanceObject *other);
		virtual void dealloc() override;
	};
}

#endif
