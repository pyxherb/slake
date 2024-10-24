#ifndef _SLAKE_OBJ_VAR_H_
#define _SLAKE_OBJ_VAR_H_

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
		SLAKE_API VarObject(Runtime *rt);
		SLAKE_API VarObject(const VarObject &x);
		SLAKE_API virtual ~VarObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Type getVarType(const VarRefContext &context) const = 0;

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &context, Value &valueOut) const = 0;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &context, const Value &value) = 0;

		SLAKE_API virtual VarKind getVarKind() const = 0;
	};

	class RegularVarObject final : public VarObject {
	public:
		Value value;
		Type type;

		std::pmr::string name;
		Object *parent = nullptr;

		SLAKE_API RegularVarObject(Runtime *rt, AccessModifier access, const Type &type);
		SLAKE_API RegularVarObject(const RegularVarObject &other);
		SLAKE_API virtual ~RegularVarObject();

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual const char *getName() const override;
		SLAKE_API virtual void setName(const char *name);
		SLAKE_API virtual Object *getParent() const override;
		SLAKE_API virtual void setParent(Object *parent);

		SLAKE_API static HostObjectRef<RegularVarObject> alloc(Runtime *rt, AccessModifier access, const Type &type);
		SLAKE_API static HostObjectRef<RegularVarObject> alloc(const RegularVarObject *other);
		SLAKE_API virtual void dealloc() override;

		/// @brief Get data of the variable.
		/// @param context Variable context for fetching data.
		/// @param valueOut Where to store the data fetched from the variable.
		/// @return true if succeeded.
		/// @return false if failed and an internal exception will be set.
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &context, Value &valueOut) const override;
		/// @brief Set data of the variable.
		/// @param context Variable context for setting data.
		/// @param value Data to be assigned to the variable.
		/// @return true if succeeded.
		/// @return false if failed and an internal exception will be set.
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &context, const Value &value) override;

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Type getVarType(const VarRefContext &context) const override { return type; }
		SLAKE_API virtual VarKind getVarKind() const override { return VarKind::Regular; }
	};

	struct LocalVarRecord {
		size_t stackOffset;
		Type type;
	};

	class LocalVarAccessorVarObject : public VarObject {
	public:
		Context *context;
		MajorFrame *majorFrame;

		SLAKE_API LocalVarAccessorVarObject(
			Runtime *rt,
			Context *context,
			MajorFrame *majorFrame);
		SLAKE_API virtual ~LocalVarAccessorVarObject();

		SLAKE_API virtual Type getVarType(const VarRefContext &context) const override;

		SLAKE_API virtual VarKind getVarKind() const override;

		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer setData(const VarRefContext &varRefContext, const Value &value) override;
		SLAKE_API [[nodiscard]] virtual InternalExceptionPointer getData(const VarRefContext &varRefContext, Value &valueOut) const override;

		SLAKE_API static HostObjectRef<LocalVarAccessorVarObject> alloc(
			Runtime *rt,
			Context *context,
			MajorFrame *majorFrame
		);
		SLAKE_API virtual void dealloc() override;
	};

	[[nodiscard]] MismatchedVarTypeError *raiseMismatchedVarTypeError(Runtime *rt);
}

#endif
