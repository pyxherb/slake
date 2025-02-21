#ifndef _SLAKE_OBJ_ROOT_H_
#define _SLAKE_OBJ_ROOT_H_

#include "object.h"
#include "member.h"

namespace slake {
	class RootObject final : public Object {
	public:
		Scope *scope;

		SLAKE_API RootObject(Runtime *rt);
		SLAKE_API virtual ~RootObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual EntityRef getMember(
			const std::string_view &name) const override;

		SLAKE_API static RootObject *alloc(Runtime *rt);

		SLAKE_API virtual void dealloc() override;
	};
}

#endif
