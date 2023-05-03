#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <deque>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>

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

	class ResourceNotFoundError : public std::runtime_error {
	public:
		inline ResourceNotFoundError(std::string msg) : runtime_error(msg){};
		virtual inline ~ResourceNotFoundError() {}
	};

	class AccessViolationError : public std::runtime_error {
	public:
		inline AccessViolationError(std::string msg) : runtime_error(msg){};
		virtual inline ~AccessViolationError() {}
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

	class LoaderError : public std::runtime_error {
	public:
		inline LoaderError(std::string msg) : runtime_error(msg){};
		virtual inline ~LoaderError() {}
	};

	class NullRefError : public std::runtime_error {
	public:
		inline NullRefError(std::string msg) : runtime_error(msg){};
		virtual inline ~NullRefError() {}
	};

	struct ExecContext final {
		ValueRef<> scopeValue;
		ValueRef<FnValue> fn;
		std::uint32_t curIns = 0;
		std::vector<ValueRef<>> args;
		ValueRef<> pThis;

		inline ExecContext() {}
		inline ExecContext(ValueRef<> scopeValue, ValueRef<FnValue> fn, std::uint32_t curIns) : scopeValue(scopeValue), fn(fn), curIns(curIns) {
		}
		inline ExecContext(const ExecContext &x) { *this = x; }
		inline ExecContext(const ExecContext &&x) { *this = x; }

		inline ExecContext &operator=(const ExecContext &&x) {
			scopeValue = x.scopeValue;
			fn = x.fn;
			curIns = x.curIns;
			args = x.args;
			return *this;
		}

		inline ExecContext &operator=(const ExecContext &x) {
			*this = std::move(x);
			return *this;
		}
	};

	struct Frame final {
		std::vector<std::uint32_t> exceptHandlers;
		std::uint32_t stackBase, exitOff;

		inline Frame(std::uint32_t stackBase, std::uint32_t exitOff = 0) : stackBase(stackBase), exitOff(exitOff) {
		}
	};

	using ContextFlag = std::uint8_t;
	constexpr static ContextFlag CTXT_YIELD = 0x01;
	struct Context final {
		ExecContext execContext;
		std::deque<ValueRef<>> dataStack;
		std::deque<Frame> frames;
		std::deque<ExecContext> callingStack;
		ValueRef<> retValue;
		std::uint32_t stackBase;

		inline Context(ExecContext execContext) : execContext(execContext), stackBase(0) {}
		virtual inline ~Context() {
		}

		inline ValueRef<> lload(std::uint32_t off) {
			if (off >= dataStack.size() - stackBase)
				throw FrameBoundaryExceededError("Frame boundary exceeded");
			return dataStack.at(stackBase + off);
		}

		inline void push(ValueRef<> v) {
			if (dataStack.size() > 0x100000)
				throw StackOverflowError("Stack overflowed");
			dataStack.push_back(v);
		}

		inline ValueRef<> pop() {
			if (dataStack.size() == stackBase)
				throw FrameBoundaryExceededError("Frame boundary exceeded");
			auto v = dataStack.back();
			dataStack.pop_back();
			return v;
		}

		inline void expand(std::uint32_t n) {
			if ((n += (std::uint32_t)dataStack.size()) > 0x100000)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}

		inline void shrink(std::uint32_t n) {
			if (n > ((std::uint32_t)dataStack.size()) || (n = ((std::uint32_t)dataStack.size()) - n) < stackBase)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}
	};

	using RuntimeFlags = std::uint16_t;
	constexpr static RuntimeFlags
		RT_NOJIT = 0x01,  // No JIT
		RT_DEBUG = 0x02	  // Enable Debugging
		;

	class Runtime final {
	private:
		void _loadScope(ModuleValue *mod, std::istream &fs);

		RootValue *_rootValue;
		std::unordered_set<Value *> _createdValues;
		RuntimeFlags _flags;

		void _execIns(Context *context, Instruction &ins);
		void _gc();
		ObjectValue *_newClassInstance(ClassValue *cls);

		void _callFn(Context *context, FnValue *fn);

		friend class Value;
		friend class FnValue;

	public:
		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		inline Runtime(RuntimeFlags flags = 0) : _flags(flags) {
			_rootValue = new RootValue(this);
			_rootValue->incRefCount();
		}
		virtual inline ~Runtime() {
			// All values were managed by the root value.
			delete _rootValue;
		}

		virtual Value *resolveRef(ValueRef<RefValue>, Value *v = nullptr);

		virtual void loadModule(std::string name, std::istream &fs);
		virtual void loadModule(std::string name, const void *buf, std::size_t size);
		inline RootValue *getRootValue() { return _rootValue; }
	};
}

#endif
