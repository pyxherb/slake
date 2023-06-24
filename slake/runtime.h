#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <sstream>
#include <thread>
#include <unordered_set>

#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "value.h"

namespace std {
	std::string to_string(Slake::Runtime &&rt);
}

namespace Slake {
	/// @brief Minor frames are created by ENTER instruction and destroyed by
	/// leave instruction.
	struct MinorFrame final {
		std::vector<uint32_t> exceptHandlers;  // Exception handlers
		uint32_t exitOff;					   // Offset of instruction to jump when leaving unexpectedly
		uint32_t stackBase;
	};

	/// @brief Each major frame represents a single calling frame.
	struct MajorFrame final {
		ValueRef<> scopeValue;						// Scope value.
		ValueRef<FnValue> curFn;					// Current function
		uint32_t curIns = 0;						// Offset of current instruction in function body
		std::vector<ValueRef<>> argStack;			// Argument stack
		std::vector<ValueRef<>> nextArgStack;		// Next Argument stack
		std::vector<ValueRef<>> dataStack;			// Data stack
		std::vector<ValueRef<VarValue>> localVars;	// Local variables
		ValueRef<> thisObject;						// `this' object
		ValueRef<> returnValue;						// Return value
		std::vector<MinorFrame> minorFrames;		// Minor frames

		inline ValueRef<> lload(uint32_t off) {
			return localVars.at(off);
		}

		inline void push(ValueRef<> v) {
			if (dataStack.size() > STACK_MAX)
				throw StackOverflowError("Stack overflowed");
			dataStack.push_back(*v);
		}

		inline ValueRef<> pop() {
			if (!dataStack.size())
				throw FrameBoundaryExceededError("Frame bottom exceeded");
			ValueRef<> v = dataStack.back();
			dataStack.pop_back();
			return v;
		}

		inline void expand(uint32_t n) {
			if ((n += (uint32_t)dataStack.size()) > STACK_MAX)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}

		inline void shrink(uint32_t n) {
			if (n > ((uint32_t)dataStack.size()))
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}
	};

	struct Context final {
		std::vector<MajorFrame> majorFrames;  // Major frames, aka calling frames

		inline MajorFrame &getCurFrame() {
			return majorFrames.back();
		}
	};

	using RuntimeFlags = uint16_t;
	constexpr static RuntimeFlags
		// No JIT, do not set unless you want to debug the JIT engine.
		RT_NOJIT = 0x01,
		// Enable Debugging, set for module debugging.
		RT_DEBUG = 0x02,
		// GC Debugging, do not set unless you want to debug the garbage collector.
		RT_GCDBG = 0x04;

	using ModuleLoaderFn = std::function<ValueRef<ModuleValue>(Runtime *rt, RefValue *ref)>;

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
		std::unordered_set<Value *> _createdValues, _extraGcTargets;
		RuntimeFlags _flags = 0;

		bool _isInGc = false;
		std::size_t _szMemInUse = 0, _szMemUsedAfterLastGc = 0;

		ModuleLoaderFn _moduleSearcher;

		void _execIns(Context *context, Instruction &ins);

		void _gcWalk(Value *i);
		ObjectValue *_newClassInstance(ClassValue *cls);

		void _callFn(Context *context, FnValue *fn);
		VarValue* _addLocalVar(MajorFrame& frame, Type type);

		friend class Value;
		friend class FnValue;
		friend class ObjectValue;
		friend class ValueRef<ObjectValue>;
		friend std::string std::to_string(Runtime &&);

	public:
		std::unordered_map<std::thread::id, std::shared_ptr<Context>> currentContexts;
		std::unordered_set<std::thread::id> destructingThreads;

		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		inline Runtime(RuntimeFlags flags = 0) : _flags(flags) {
			_rootValue = new RootValue(this);
			_rootValue->incRefCount();
		}
		virtual inline ~Runtime() {
			gc();

			// Execute destructors for all destructible objects.
			destructingThreads.insert(std::this_thread::get_id());
			for (auto i : _createdValues) {
				auto d = i->getMember("delete");
				if (d && i->getType() == ValueType::OBJECT)
					d->call(0, nullptr);
			}
			destructingThreads.erase(std::this_thread::get_id());

			while (_createdValues.size())
				delete *_createdValues.begin();
		}

		virtual Value *resolveRef(ValueRef<RefValue>, Value *v = nullptr);

		virtual ValueRef<ModuleValue> loadModule(std::istream &fs, std::string name = "");
		virtual ValueRef<ModuleValue> loadModule(const void *buf, std::size_t size, std::string name = "");
		inline RootValue *getRootValue() { return _rootValue; }

		inline void setModuleLoader(ModuleLoaderFn searcher) { _moduleSearcher = searcher; }
		inline ModuleLoaderFn getModuleLoader() { return _moduleSearcher; }

		inline std::string resolveName(MemberValue *v) {
			std::string s;
			do {
				s = v->getName() + (s.empty() ? "::" + s : "");
			} while ((Value *)(v = (MemberValue *)v->getParent()) != _rootValue);
			return s;
		}

		inline std::shared_ptr<Context> getCurContext() {
			return currentContexts.count(std::this_thread::get_id())
					   ? currentContexts.at(std::this_thread::get_id())
					   : throw std::logic_error("Current thread is not executing Slake functions");
		}

		void gc();
	};
}

namespace std {
	/*
	inline std::string to_string(Slake::MinorFrame &&ctxt) {
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
	inline std::string to_string(Slake::Context &ctxt) {
		return to_string(move(ctxt));
	}*/

	inline std::string to_string(Slake::Runtime &&rt) {
		std::string s =
			"{\"rootValue\":" + std::to_string((uintptr_t)rt._rootValue) +
			",\"inGc\":" + std::to_string(rt._isInGc) +
			",\"_szMemInUse\":" + std::to_string(rt._szMemInUse) +
			",\"szLastGcUsed\":" + std::to_string(rt._szMemUsedAfterLastGc) +
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
