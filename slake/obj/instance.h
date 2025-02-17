#ifndef _SLAKE_OBJ_INSTANCE_H_
#define _SLAKE_OBJ_INSTANCE_H_

#include <unordered_map>
#include <deque>

#include "var.h"
#include "generic.h"

namespace slake {
	using InstanceFlags = uint32_t;

	struct ObjectLayout;

	class InstanceObject final : public Object {
	public:
		ClassObject *_class = nullptr;
		char *rawFieldData = nullptr;
		size_t szRawFieldData = 0;

		InstanceFlags instanceFlags = 0;

		SLAKE_API InstanceObject(Runtime *rt);
		SLAKE_API InstanceObject(const InstanceObject &x);
		SLAKE_API virtual ~InstanceObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual ObjectRef getMember(
			const std::string_view &name) const override;

		SLAKE_API static HostObjectRef<InstanceObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<InstanceObject> alloc(const InstanceObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
