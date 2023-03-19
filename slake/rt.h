#ifndef _SLAKE_RT_H_
#define _SLAKE_RT_H_

#include <deque>
#include <functional>
#include <unordered_map>
#include <string>

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

	class FrameError : public std::runtime_error {
	public:
		inline FrameError(std::string msg) : runtime_error(msg){};
		virtual inline ~FrameError() {}
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
		Instruction* _body;
		std::uint32_t _nIns;

	public:
		Fn(FnFlags flags, std::size_t nIns = 0);
		virtual ~Fn();

		std::uint32_t getInsCount() const;
		const Instruction* getBody() const;
		Instruction* getBody();
		FnFlags getFlags() const;

		inline bool isPublic() const { return getFlags() & FN_PUB; }
		inline bool isFinal() const { return getFlags() & FN_FINAL; }
		inline bool isOverriden() const { return getFlags() & FN_FINAL; }
		inline bool isNative() const { return getFlags() & FN_FINAL; }

		Fn& operator=(const Fn&) = delete;
		Fn& operator=(const Fn&&) = delete;
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

	struct ExecContext final {
		Module* mod;
		Fn* fn;
		std::uint32_t curIns;
		Object* gpRegs[8];

		inline ExecContext() {}
		inline ExecContext(Module* mod, Fn* fn, std::uint32_t curIns) : mod(mod), fn(fn), curIns(curIns) {}
	};

	struct Frame final {
		std::vector<ExecContext> exceptHandlers;
		std::uint32_t stackBase, exitOff;

		inline Frame(std::uint32_t stackBase, std::uint32_t exitOff = 0) : stackBase(stackBase), exitOff(exitOff) {
		}
	};

	class Context {
	public:
		ExecContext execContext;
		std::deque<ObjectRef<>> dataStack;
		std::deque<Frame> frames;
		std::deque<ExecContext> callingStack;
		std::uint32_t stackBase;

		inline Context(ExecContext execContext) : execContext(execContext), stackBase(0) {}
		virtual inline ~Context() {}

		inline ObjectRef<> lload(std::uint32_t off) {
			if (off >= dataStack.size() - stackBase)
				throw FrameBoundaryExceededError("Frame boundary exceeded");
			return dataStack.at(stackBase + off);
		}

		inline void push(ObjectRef<>& ref) {
			if (dataStack.size() > 0x100000)
				throw StackOverflowError("Stack overflowed");
			dataStack.push_back(ref);
		}

		inline void push(ObjectRef<>&& ref) {
			push(ref);
		}

		inline Slake::ObjectRef<> pop() {
			if ((dataStack.size() - 1) < stackBase)
				throw FrameBoundaryExceededError("Frame boundary exceeded");
			auto v = dataStack.back();
			dataStack.pop_back();
			return v;
		}

		inline void expand(std::uint32_t n) {
			if ((n += dataStack.size()) > 0x100000)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}

		inline void shrink(std::uint32_t n) {
			if (n > dataStack.size() || (n = dataStack.size() - n) < stackBase)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}
	};

	class Runtime {
	protected:
		std::unordered_map<std::string, Module*> modules;
		void execIns(Context* context, Instruction* ins);

	public:
		Runtime(Runtime&) = delete;
		Runtime(Runtime&&) = delete;
		Runtime& operator=(Runtime&) = delete;
		Runtime& operator=(Runtime&&) = delete;

		virtual inline ~Runtime() {}

		virtual Module* getModule(std::string name) = 0;
		Object* resolveRef(RefObject* ref, Module* mod = nullptr);
	};

	Module* loadModule(const void* src, std::size_t size);
}

#endif
