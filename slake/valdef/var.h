#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	enum class VarKind {
		Regular = 0,
		ArrayElementAccessor,
		InstanceMemberAccessor
	};

	class VarObject : public MemberObject {
	public:
		VarObject(Runtime *rt);
		inline VarObject(const VarObject &x) : MemberObject(x) {
		}
		virtual ~VarObject();

		virtual inline ObjectKind getKind() const override { return ObjectKind::Var; }

		virtual Type getVarType(const VarRefContext &context) const = 0;

		virtual Value getData(const VarRefContext &context) const = 0;
		virtual void setData(const VarRefContext &context, const Value &value) = 0;

		virtual VarKind getVarKind() const = 0;
	};

	class RegularVarObject final : public VarObject {
	public:
		Value value;
		Type type;

		std::pmr::string name;
		Object *parent = nullptr;

		RegularVarObject(Runtime *rt, AccessModifier access, const Type &type);
		inline RegularVarObject(const RegularVarObject &other) : VarObject(other) {
			value = other.value;
			type = other.type;

			name = other.name;
			parent = other.parent;
		}
		virtual ~RegularVarObject();

		virtual Object *duplicate() const override;

		virtual const char *getName() const override;
		virtual void setName(const char *name);
		virtual Object *getParent() const override;
		virtual void setParent(Object *parent);

		static HostObjectRef<RegularVarObject> alloc(Runtime *rt, AccessModifier access, const Type &type);
		static HostObjectRef<RegularVarObject> alloc(const RegularVarObject *other);
		virtual void dealloc() override;

		virtual inline Value getData(const VarRefContext &context) const override { return value; }
		virtual inline void setData(const VarRefContext &context, const Value &value) override {
			if (!isCompatible(type, value))
				throw MismatchedTypeError("Mismatched variable type");
			this->value = value;
		}

		virtual inline ObjectKind getKind() const override { return ObjectKind::Var; }

		virtual Type getVarType(const VarRefContext &context) const override { return type; }
		virtual VarKind getVarKind() const override { return VarKind::Regular; }
	};
}

#endif
