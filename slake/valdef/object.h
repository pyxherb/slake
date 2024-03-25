#ifndef _SLAKE_VALDEF_OBJECT_H_
#define _SLAKE_VALDEF_OBJECT_H_

#include <unordered_map>
#include <deque>

#include "member.h"
#include "generic.h"

namespace slake {
	using ObjectFlags = uint32_t;

	constexpr static ObjectFlags
		OBJECT_PARENT = 0x01;

	class ObjectValue final : public Value {
	protected:
		GenericArgList _genericArgs;
		ClassValue *_class;

		friend class Runtime;

	public:
		ObjectValue *_parent;

		ObjectFlags objectFlags = 0;

		inline ObjectValue(Runtime *rt, ClassValue *cls, ObjectValue *parent = nullptr)
			: Value(rt), _class(cls), _parent(parent) {
			if (parent)
				parent->objectFlags |= OBJECT_PARENT;
			scope = new Scope(this, parent ? parent->scope : nullptr);
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
		}

		/// @brief Delete the object and execute its destructor (if exists).
		///
		/// @note Never delete objects directly.
		virtual inline ~ObjectValue() {
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual inline Type getType() const override { return Type(TypeId::Object, (Value *)_class); }

		virtual Value *duplicate() const override;

		ObjectValue(ObjectValue &) = delete;
		ObjectValue(ObjectValue &&) = delete;
		inline ObjectValue &operator=(const ObjectValue &x) {
			(Value &)*this = (const Value &)x;

			_genericArgs = x._genericArgs;
			_class = x._class;
			objectFlags = x.objectFlags & ~OBJECT_PARENT;

			return *this;
		}
		ObjectValue &operator=(ObjectValue &&) = delete;
	};
}

#endif
