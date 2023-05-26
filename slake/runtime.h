#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <deque>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>

#include "opcode.h"
#include "value.h"

namespace std {
	std::string to_string(Slake::Runtime &&rt);
}

namespace Slake {
	struct MinorFrame {
		std::vector<uint32_t> exceptHandlers;
	};

	struct MajorFrame {
		ValueRef<> scopeValue;
		ValueRef<FnValue> curFn;
		uint32_t curIns = 0;
		std::vector<ValueRef<>> argStack;
		ValueRef<> thisReg;
	};

	struct XContext {
		std::vector<MajorFrame> frames;
	};

	struct ExecContext final {
		ValueRef<> scopeValue;
		ValueRef<FnValue> fn;
		uint32_t curIns = 0;
		std::vector<ValueRef<>> args;
		ValueRef<> pThis;

		inline ExecContext() {}
		inline ExecContext(ValueRef<> &&scopeValue, ValueRef<FnValue> &&fn, uint32_t curIns) : scopeValue(scopeValue), fn(fn), curIns(curIns) {
		}
		inline ExecContext(const ExecContext &&x) noexcept { *this = x; }
		inline ExecContext(const ExecContext &x) noexcept { *this = x; }

		inline ExecContext &operator=(const ExecContext &&x) noexcept {
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
		std::vector<uint32_t> exceptHandlers;
		uint32_t stackBase, exitOff;

		inline Frame(uint32_t stackBase, uint32_t exitOff = 0) : stackBase(stackBase), exitOff(exitOff) {
		}
	};

	using ContextFlag = uint8_t;
	constexpr static ContextFlag CTXT_YIELD = 0x01;
	struct Context final {
		ExecContext execContext;
		std::deque<ValueRef<>> dataStack;
		std::deque<Frame> frames;
		std::deque<ExecContext> callingStack;
		ValueRef<> retValue;
		uint32_t stackBase;

		inline Context(ExecContext execContext) : execContext(execContext), stackBase(0) {}
		virtual inline ~Context() {
		}

		inline ValueRef<> lload(uint32_t off) {
			if (off >= dataStack.size() - stackBase)
				throw FrameBoundaryExceededError("Frame boundary exceeded");
			return dataStack.at((size_t)stackBase + off);
		}

		inline void push(ValueRef<> v) {
			if (dataStack.size() > 0x100000)
				throw StackOverflowError("Stack overflowed");
			dataStack.push_back(*v);
		}

		inline ValueRef<> pop() {
			if (dataStack.size() == stackBase)
				throw FrameBoundaryExceededError("Frame boundary exceeded");
			ValueRef<> v = dataStack.back();
			dataStack.pop_back();
			return v;
		}

		inline void expand(uint32_t n) {
			if ((n += (uint32_t)dataStack.size()) > 0x100000)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}

		inline void shrink(uint32_t n) {
			if (n > ((uint32_t)dataStack.size()) || (n = ((uint32_t)dataStack.size()) - n) < stackBase)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}
	};

	using RuntimeFlags = uint16_t;
	constexpr static RuntimeFlags
		RT_NOJIT = 0x01,  // No JIT
		RT_DEBUG = 0x02	  // Enable Debugging
		;

	using ModuleSearcherFn = std::function<ValueRef<ModuleValue>(Runtime *rt, RefValue *ref)>;

	inline ValueRef<RefValue> parseRef(Runtime *rt, std::string ref) {
		ValueRef<RefValue> v(new RefValue(rt));
		std::string curScope;

		for (size_t i = 0; i < ref.length(); i++) {
			switch (ref[i]) {
				case ':':
					if (ref[i + 1] != ':')
						throw RefParseError(std::to_string(i) + ": Expecting ':'");
					return nullptr;
					i++;

				case '.':
					v->scopes.push_back(curScope);
					curScope.clear();
					continue;
				default:
					curScope += ref[i];
			}
		}

		return v;
	}

	class Runtime final {
	private:
		void _loadScope(ModuleValue *mod, std::istream &fs);

		RootValue *_rootValue;
		std::unordered_set<Value *> _createdValues;
		RuntimeFlags _flags = 0;

		std::size_t szInUse = 0, szLastGcMemUsed = 0;

		constexpr static uint8_t RTI_LASTGCSTAT = 0x01;
		uint8_t _internalFlags = RTI_LASTGCSTAT;

		void _execIns(Context *context, Instruction &ins);

		void _gcWalk(Value *i, uint8_t gcStat);
		void _gcWalkForExecContext(ExecContext &ctxt, uint8_t gcStat);
		ObjectValue *_newClassInstance(ClassValue *cls);

		void _callFn(Context *context, FnValue *fn);

		bool isInGc = false;

		ModuleSearcherFn _moduleSearcher;

		friend class Value;
		friend class FnValue;
		friend std::string std::to_string(Runtime &&);

	public:
		std::unordered_map<std::thread::id, std::shared_ptr<Context>> threadCurrentContexts;

		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		inline Runtime(RuntimeFlags flags = 0) : _flags(flags) {
			_rootValue = new RootValue(this);
		}
		virtual inline ~Runtime() {
			// All values were managed by the root value.
			delete _rootValue;
			_rootValue = nullptr;
			gc();
		}

		virtual Value *resolveRef(ValueRef<RefValue>, Value *v = nullptr);

		virtual ValueRef<ModuleValue> loadModule(std::istream &fs);
		virtual ValueRef<ModuleValue> loadModule(const void *buf, std::size_t size);
		inline RootValue *getRootValue() { return _rootValue; }

		inline void setModuleSearcher(ModuleSearcherFn searcher) { _moduleSearcher = searcher; }
		inline ModuleSearcherFn getModuleSearcher() { return _moduleSearcher; }

		void gc();
	};
}

namespace std {
	inline std::string to_string(Slake::ExecContext &&ctxt) {
		std::string s = "{\"curFn\":" + std::to_string((uintptr_t)*ctxt.fn) +
						",\"curIns\":" + std::to_string(ctxt.curIns) +
						",\"pThis\":" + std::to_string((uintptr_t)*ctxt.pThis) +
						",\"scopeValue\":" + std::to_string((uintptr_t)*ctxt.scopeValue);
		s += ",\"args\":[";

		for (size_t i = 0; i != ctxt.args.size(); ++i) {
			s += (i ? ",\"" : "\"") + std::to_string((uintptr_t)*ctxt.args[i]);
		}

		s += "]}";

		return s;
	}

	inline std::string to_string(Slake::ExecContext &ctxt) {
		return to_string(move(ctxt));
	}

	inline std::string to_string(Slake::Context &&ctxt) {
		std::string s = "{\"retValue\":" + std::to_string((uintptr_t)ctxt.retValue) +
						",\"stackBase\":" + std::to_string(ctxt.stackBase) +
						",\"execContext\":" + std::to_string(ctxt.execContext);

		s += ",\"callingStack\":[";

		for (size_t i = 0; i != ctxt.callingStack.size(); ++i) {
			s += (i ? "," : "") + std::to_string(ctxt.callingStack[i]);
		}

		s += "],\"dataStack\":[";

		for (size_t i = 0; i != ctxt.dataStack.size(); ++i) {
			s += (i ? "," : "") + std::to_string((uintptr_t)*ctxt.dataStack[i]);
		}

		s += "]}";

		return s;
	}
	inline std::string to_string(Slake::Context& ctxt) {
		return to_string(move(ctxt));
	}

	inline std::string to_string(Slake::Runtime &&rt) {
		std::string s =
			"{\"rootValue\":" + std::to_string((uintptr_t)rt._rootValue) +
			",\"inGc\":" + std::to_string(rt.isInGc) +
			",\"szInUse\":" + std::to_string(rt.szInUse) +
			",\"szLastGcUsed\":" + std::to_string(rt.szLastGcMemUsed) +
			",\"flags\":" + std::to_string(rt._flags) +
			",\"moduleSearcherType\":\"" + rt._moduleSearcher.target_type().name() + "\"";

		s += ",\"createdValues\":{";
		for (auto i = rt._createdValues.begin(); i != rt._createdValues.end(); ++i) {
			s += (i != rt._createdValues.begin() ? ",\"" : "\"") + std::to_string((uintptr_t)*i) + "\":" + std::to_string(*i);
		}
		s += "}";

		s += "}";

		return s;
	}
	inline std::string to_string(Slake::Runtime &rt) {
		return std::to_string(move(rt));
	}
	inline std::string to_string(Slake::Runtime *rt) {
		return std::to_string(*rt);
	}
}

#endif
