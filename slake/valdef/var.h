#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	struct MajorFrame;
	struct Context;

	enum class VarKind {
		Regular = 0,
		ArrayElementAccessor,
		InstanceMemberAccessor,
		LocalVarAccessor
	};

	class VarObject : public MemberObject {
	public:
		VarObject(Runtime *rt);
		VarObject(const VarObject &x);
		virtual ~VarObject();

		virtual ObjectKind getKind() const override;

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
		RegularVarObject(const RegularVarObject &other);
		virtual ~RegularVarObject();

		virtual Object *duplicate() const override;

		virtual const char *getName() const override;
		virtual void setName(const char *name);
		virtual Object *getParent() const override;
		virtual void setParent(Object *parent);

		static HostObjectRef<RegularVarObject> alloc(Runtime *rt, AccessModifier access, const Type &type);
		static HostObjectRef<RegularVarObject> alloc(const RegularVarObject *other);
		virtual void dealloc() override;

		virtual Value getData(const VarRefContext &context) const override;
		virtual void setData(const VarRefContext &context, const Value &value) override;

		virtual ObjectKind getKind() const override;

		virtual Type getVarType(const VarRefContext &context) const override { return type; }
		virtual VarKind getVarKind() const override { return VarKind::Regular; }
	};

	struct LocalVarRecord {
		size_t stackOffset;
		Type type;
	};

	class LocalVarAccessorVarObject : public VarObject {
	public:
		Context *context;
		MajorFrame *majorFrame;

		LocalVarAccessorVarObject(
			Runtime *rt,
			Context *context,
			MajorFrame *majorFrame);
		virtual ~LocalVarAccessorVarObject();

		virtual Type getVarType(const VarRefContext &context) const override;

		virtual VarKind getVarKind() const override;

		virtual void setData(const VarRefContext &varRefContext, const Value &value) override;
		virtual Value getData(const VarRefContext &varRefContext) const override;

		static HostObjectRef<LocalVarAccessorVarObject> alloc(
			Runtime *rt,
			Context *context,
			MajorFrame *majorFrame
		);
		virtual void dealloc() override;
	};
}

#endif
