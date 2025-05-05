#ifndef _SLAKE_OBJ_SCOPE_H_
#define _SLAKE_OBJ_SCOPE_H_

#include <peff/containers/hashmap.h>
#include <peff/containers/string.h>
#include <peff/containers/list.h>
#include <memory>
#include <string>
#include <string_view>
#include <slake/util/memory.h>
#include <slake/basedefs.h>

namespace slake {
	class Object;
	class MemberObject;
	class FnObject;
	class FnOverloadingObject;

	class InstanceObject;

	typedef void (*ClassNativeDestructor)(InstanceObject *instanceObject);

}

#endif
