#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <sstream>
#include <thread>
#include <unordered_set>
#include <memory>

#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "value.h"

namespace slake {
	/// @brief Minor frames which are created by ENTER instructions and
	/// destroyed by LEAVE instructions.
	struct MinorFrame final {
		std::deque<uint32_t> exceptHandlers;  // Exception handlers
		uint32_t exitOff;					  // Offset of instruction to jump when leaving unexpectedly
		uint32_t stackBase;
	};

	/// @brief Major frames which represent a single calling frame.
	struct MajorFrame final {
		ValueRef<> scopeValue;					   // Scope value.
		ValueRef<const FnValue> curFn;				   // Current function
		uint32_t curIns = 0;					   // Offset of current instruction in function body
		std::deque<ValueRef<>> argStack;		   // Argument stack
		std::deque<ValueRef<>> nextArgStack;	   // Next Argument stack
		std::deque<ValueRef<>> dataStack;		   // Data stack
		std::deque<ValueRef<VarValue>> localVars;  // Local variables
		ValueRef<> thisObject;					   // `this' object
		ValueRef<> returnValue;					   // Return value
		std::deque<MinorFrame> minorFrames;		   // Minor frames

		inline ValueRef<> lload(uint32_t off) {
			return localVars.at(off);
		}

		inline void push(ValueRef<> v) {
			if (dataStack.size() > SLAKE_STACK_MAX)
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
			if ((n += (uint32_t)dataStack.size()) > SLAKE_STACK_MAX)
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
		std::deque<MajorFrame> majorFrames;	 // Major frames, aka calling frames

		inline MajorFrame &getCurFrame() {
			return majorFrames.back();
		}
	};

	using RuntimeFlags = uint32_t;
	constexpr static RuntimeFlags
		// No JIT, do not set unless you want to debug the JIT engine.
		RT_NOJIT = 0x0000001,
		// Enable Debugging, set for module debugging.
		RT_DEBUG = 0x0000002,
		// GC Debugging, do not set unless you want to debug the garbage collector.
		RT_GCDBG = 0x0000004,
		// The runtime is deleting by user.
		_RT_DELETING = 0x80000000;

	using ModuleLoaderFn = std::function<ValueRef<ModuleValue>(Runtime *rt, RefValue *ref)>;

	class Runtime final {
	private:
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
		VarValue *_addLocalVar(MajorFrame &frame, Type type);

		friend class Value;
		friend class FnValue;
		friend class ObjectValue;
		friend class MemberValue;
		friend class ModuleValue;
		friend class ValueRef<ObjectValue>;

	public:
		std::unordered_map<std::thread::id, std::shared_ptr<Context>> currentContexts;
		std::unordered_set<std::thread::id> destructingThreads;

		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		Runtime(RuntimeFlags flags = 0);
		virtual ~Runtime();

		virtual Value *resolveRef(ValueRef<RefValue>, Value *scopeValue = nullptr) const;

		virtual ValueRef<ModuleValue> loadModule(std::istream &fs, std::string name = "");
		virtual ValueRef<ModuleValue> loadModule(const void *buf, std::size_t size, std::string name = "");
		inline RootValue *getRootValue() { return _rootValue; }

		inline void setModuleLoader(ModuleLoaderFn searcher) { _moduleSearcher = searcher; }
		inline ModuleLoaderFn getModuleLoader() { return _moduleSearcher; }

		std::string resolveName(const MemberValue *v) const;

		inline std::shared_ptr<Context> getCurContext() {
			return currentContexts.count(std::this_thread::get_id())
					   ? currentContexts.at(std::this_thread::get_id())
					   : throw std::logic_error("Current thread is not handled by Slake runtime");
		}

		void gc();

		std::string getMangledFnName(std::string name, std::deque<Type> params);
	};
}

#endif
