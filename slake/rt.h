#ifndef _SLAKE_RT_H_
#define _SLAKE_RT_H_

#include <functional>
#include <slake/base/uuid.hh>
#include <unordered_map>

#include "context.h"
#include "opcode.h"
#include "value.h"

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

	using NativeFn = std::function<void()>;

	struct NativeVarRegistry final {
		std::function<void(std::shared_ptr<Value> value)> read;
		std::function<void(std::shared_ptr<Value> value)> write;
	};

	struct Instruction final {
		Opcode opcode;
		std::shared_ptr<Value> operands[3];

		inline bool getOperandCount() {
			for (std::uint8_t i=0;i<4;i++)
				if (!operands[i])
					return i;
			throw std::logic_error("Error counting operands");
		}
	};

	using FnFlags = std::uint16_t;
	constexpr static FnFlags
		FN_PUB = 0x0001,
		FN_FINAL = 0x0002,
		FN_OVERRIDE = 0x0004,
		FN_STATIC = 0x0008,
		FN_NATIVE = 0x0010;

	class Fn {
	public:
		inline Fn() {}
		virtual inline ~Fn() {}

		virtual std::size_t getInsCount() const = 0;
		virtual const Instruction& getIns(std::size_t i) const = 0;
		virtual std::string getName() const = 0;
		virtual FnFlags getFlags() const = 0;

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

		virtual const std::shared_ptr<Fn> getFn(std::string name) = 0;
		virtual void registerNativeFn(std::string name, const NativeFn fn) = 0;
		virtual void unregisterNativeFn(std::string name) = 0;

		virtual void registerNativeVar(std::string name, const NativeVarRegistry& registry) = 0;
		virtual void unregisterNativeVar(std::string name) = 0;
	};

	class Module {
	public:
		inline Module() {}
		Module(Module&) = delete;
		Module(Module&&) = delete;
		Module& operator=(Module&) = delete;
		Module& operator=(Module&&) = delete;

		virtual inline ~Module() {}

		virtual const std::shared_ptr<Fn> getFn(std::string name) = 0;

		virtual void registerNativeVar(std::string name, const NativeVarRegistry& registry) = 0;
		virtual void unregisterNativeVar(std::string name) = 0;
	};

	class Runtime {
	public:
		Runtime(Runtime&) = delete;
		Runtime(Runtime&&) = delete;
		Runtime& operator=(Runtime&) = delete;
		Runtime& operator=(Runtime&&) = delete;

		virtual inline ~Runtime() {}

		virtual std::shared_ptr<Module> getModule(std::string name) = 0;
	};

	std::shared_ptr<Module> loadModule(const void* src, std::size_t size);
}

#endif
