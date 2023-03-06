#ifndef _SLAKE_RT_H_
#define _SLAKE_RT_H_

#include <deque>
#include <functional>
#include <slake/base/uuid.hh>
#include <unordered_map>

#include "base/resptr.h"
#include "object.h"
#include "opcode.h"

namespace Slake {
	class InvalidOpcodeError : public std::runtime_error {
	public:
		inline InvalidOpcodeError(std::string msg) : runtime_error(msg){};
		virtual inline ~InvalidOpcodeError() {}
	};

	class InvalidOperandsError : public std::runtime_error {
	public:
		inline InvalidOperandsError(std::string msg) : runtime_error(msg){};
		virtual inline ~InvalidOperandsError() {}
	};

	class UncaughtExceptionError : public std::runtime_error {
	public:
		inline UncaughtExceptionError(std::string msg) : runtime_error(msg){};
		virtual inline ~UncaughtExceptionError() {}
	};

	class AbortedError : public std::runtime_error {
	public:
		inline AbortedError(std::string msg) : runtime_error(msg){};
		virtual inline ~AbortedError() {}
	};

	class FrameBoundaryExceededError : public std::runtime_error {
	public:
		inline FrameBoundaryExceededError(std::string msg) : runtime_error(msg){};
		virtual inline ~FrameBoundaryExceededError() {}
	};

	class StackOverflowError : public std::runtime_error {
	public:
		inline StackOverflowError(std::string msg) : runtime_error(msg){};
		virtual inline ~StackOverflowError() {}
	};

	struct Instruction final {
		Opcode opcode;
		Object* operands[3];

		inline std::uint8_t getOperandCount() {
			if (!operands[0])
				return 0;
			if (!operands[1])
				return 1;
			if (!operands[2])
				return 2;
			return 3;
		}
	};

	using VarFlags = std::uint8_t;
	constexpr static VarFlags
		VAR_PUB = 0x01,
		VAR_FINAL = 0x02,
		VAR_STATIC = 0x04,
		VAR_NATIVE = 0x08;

	class Var {
	protected:
		Object* _value;

	public:
		inline Var() {}
		virtual inline ~Var() {}

		virtual ObjectRef<> getValue() = 0;
		virtual const ObjectRef<> getValue() const = 0;
		virtual void setValue(ObjectRef<> value) = 0;

		virtual VarFlags getFlags() const = 0;
		inline bool isPublic() const { return getFlags() & VAR_PUB; }
	};

	using FnFlags = std::uint8_t;
	constexpr static FnFlags
		FN_PUB = 0x01,
		FN_FINAL = 0x02,
		FN_OVERRIDE = 0x04,
		FN_STATIC = 0x08,
		FN_NATIVE = 0x10;

	class Fn {
	protected:
		FnFlags _flags;
		union {
			struct {
				Instruction* _body;
				std::uint32_t _nIns;
			} normal;
			std::function<void()> native;
		} _body;

	public:
		Fn(FnFlags flags, std::size_t nIns = 0);
		virtual ~Fn();

		std::uint32_t getInsCount() const;
		const Instruction* getBody() const;
		Instruction* getBody();
		const std::function<void()>& getNativeBody() const;
		std::function<void()>& getNativeBody();
		FnFlags getFlags() const;

		inline bool isPublic() const { return getFlags() & FN_PUB; }
		inline bool isFinal() const { return getFlags() & FN_FINAL; }
		inline bool isOverriden() const { return getFlags() & FN_FINAL; }
		inline bool isNative() const { return getFlags() & FN_FINAL; }
	};

	class Class {
	public:
		inline Class() {}
		Class(Class&) = delete;
		Class(Class&&) = delete;
		Class& operator=(Class&) = delete;
		Class& operator=(Class&&) = delete;

		virtual const Var* getVar(std::string name) const = 0;
		virtual Var* getVar(std::string name) = 0;
		virtual void addVar(std::string name, Fn* fn) = 0;
		virtual void forEachVar(std::function<Fn*> callback) = 0;

		virtual const Fn* getFn(std::string name) const = 0;
		virtual Fn* getFn(std::string name) = 0;
		virtual void addFn(std::string name, Fn* fn) = 0;
		virtual void forEachFn(std::function<Fn*> callback) = 0;

		virtual const Fn* getMethod(std::string name) const = 0;
		virtual Fn* getMethod(std::string name) = 0;
		virtual void addMethod(std::string name, Fn* fn) = 0;
		virtual void forEachMethod(std::function<Fn*> callback) = 0;
	};

	class Module {
	protected:
		std::unordered_map<std::string, Fn*> _functions, _vars;

	public:
		inline Module() {}
		Module(Module&) = delete;
		Module(Module&&) = delete;
		Module& operator=(Module&) = delete;
		Module& operator=(Module&&) = delete;

		virtual inline ~Module() {}

		virtual const Fn* getFn(std::string name);
		virtual const Var* getVar(std::string name);
	};

	class Runtime {
	protected:
		std::unordered_map<std::string, Module*> modules;

	public:
		Runtime(Runtime&) = delete;
		Runtime(Runtime&&) = delete;
		Runtime& operator=(Runtime&) = delete;
		Runtime& operator=(Runtime&&) = delete;

		virtual inline ~Runtime() {}

		virtual Module* getModule(std::string name) = 0;
		void execIns(Context* context, Instruction* ins);
		Object* resolveRef(RefValue* ref, Module* mod = nullptr);
	};

	Module* loadModule(const void* src, std::size_t size);
}

#endif
