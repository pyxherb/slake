#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <sstream>
#include <thread>
#include <unordered_set>
#include <set>
#include <memory>
#include <slake/slxfmt.h>

#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "value.h"
#include "dbg/adapter.h"

namespace slake {
	struct ExceptionHandler final {
		Type type;
		uint32_t off;
	};

	/// @brief Minor frames which are created by ENTER instructions and
	/// destroyed by LEAVE instructions.
	struct MinorFrame final {
		std::deque<ExceptionHandler> exceptHandlers;  // Exception handlers

		std::deque<ValueRef<>> dataStack;  // Data stack

		MinorFrame(Runtime *rt);

		inline void push(ValueRef<> v) {
			if (dataStack.size() > SLAKE_STACK_MAX)
				throw StackOverflowError("Stack overflowed");
			dataStack.push_back(v.get());
		}

		inline ValueRef<> pop() {
			if (!dataStack.size())
				throw FrameBoundaryExceededError("Frame bottom exceeded");
			ValueRef<> v = dataStack.back();
			dataStack.pop_back();
			return v;
		}
	};

	/// @brief Major frames which represent a single calling frame.
	struct MajorFrame final {
		ValueRef<> scopeValue;					   // Scope value.
		ValueRef<const FnValue> curFn;			   // Current function
		uint32_t curIns = 0;					   // Offset of current instruction in function body
		std::deque<ValueRef<VarValue>> argStack;   // Argument stack
		std::deque<ValueRef<>> nextArgStack;	   // Next Argument stack
		std::deque<ValueRef<VarValue>> localVars;  // Local variables
		std::deque<ValueRef<VarValue>> regs;	   // Local registers
		ValueRef<> thisObject;					   // `this' object
		ValueRef<> returnValue;					   // Return value
		std::deque<MinorFrame> minorFrames;		   // Minor frames
		ValueRef<> curExcept;					   // Current exception

		MajorFrame(Runtime *rt);

		inline ValueRef<> lload(uint32_t off) {
			if (off >= localVars.size())
				throw InvalidLocalVarIndexError("Invalid local variable index", off);

			return localVars.at(off).get();
		}

		/// @brief Leave current minor frame.
		inline void leave() {
			minorFrames.pop_back();
		}
	};

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Context execution has done (cannot be resumed).
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context final {
		std::deque<MajorFrame> majorFrames;	 // Major frames, aka calling frames
		ContextFlags flags = 0;				 // Flags

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
		// Enable GC Debugging, do not set unless you want to debug the garbage collector.
		RT_GCDBG = 0x0000004,
		// Enable strict mode
		RT_STRICT = 0x00000008,
		// The runtime is in a GC cycle.
		_RT_INGC = 0x40000000,
		// The runtime is destructing.
		_RT_DELETING = 0x80000000;

	using ModuleLocatorFn = std::function<
		std::unique_ptr<std::istream>(Runtime *rt, ValueRef<RefValue> ref)>;

	using LoadModuleFlags = uint8_t;
	constexpr LoadModuleFlags
		// Do not put the module onto the path where corresponds to the module name.
		LMOD_NOMODNAME = 0x01,
		// Return if module that corresponds to the module name exists.
		LMOD_RETIFEXISTS = 0x02,
		// Throw an exception if module that corresponds to the module name exists.
		LMOD_NOCONFLICT = 0x04;

	class Runtime final {
	private:
		/// @brief Root value of the runtime.
		RootValue *_rootValue;

		/// @brief Contains all created values.
		std::set<Value *> _createdValues;

		/// @brief Extra target values for GC, all contained values will be released by the
		/// garbage collector every GC cycle and this container will be cleared when the cycle
		/// was completed.
		std::set<Value *> _extraGcTargets;

		struct GenericLookupEntry {
			Value *originalValue;
			GenericArgList genericArgs;
		};
		std::map<Value *, GenericLookupEntry> _genericCacheLookupTable;

		using GenericCacheTable =
			std::map<
				GenericArgList,	 // Generic arguments.
				Value *,		 // Cached instantiated value.
				GenericArgListComparator>;

		using GenericCacheDirectory = std::map<
			const Value *,	// Original generic value.
			GenericCacheTable>;

		/// @brief Cached instances of generic values.
		mutable GenericCacheDirectory _genericCacheDir;

		inline void invalidateGenericCache(Value *i) {
			if (_genericCacheLookupTable.count(i)) {
				// Remove the value from generic cache if it is unreachable.
				auto &lookupEntry = _genericCacheLookupTable.at(i);

				auto &table = _genericCacheDir.at(lookupEntry.originalValue);
				table.erase(lookupEntry.genericArgs);

				if (!table.size())
					_genericCacheLookupTable.erase(lookupEntry.originalValue);

				_genericCacheLookupTable.erase(i);
			}
		}

		/// @brief Size of memory allocated for values.
		size_t _szMemInUse = 0;
		/// @brief Size of memory allocated for values after last GC cycle.
		size_t _szMemUsedAfterLastGc = 0;

		/// @brief Module locator for importing.
		ModuleLocatorFn _moduleLocator;

		RefValue *_loadRef(std::istream &fs);
		Value *_loadValue(std::istream &fs);
		Type _loadType(std::istream &fs, slxfmt::Type vt);
		GenericParam _loadGenericParam(std::istream &fs);
		void _loadScope(ModuleValue *mod, std::istream &fs);

		/// @brief Execute a single instruction.
		/// @param context Context for execution.
		/// @param ins Instruction to be executed.
		///
		/// @note Opcode-callback map was not introduced because designated initialization
		/// was not introduced into ISO C++17.
		void _execIns(Context *context, Instruction ins);

		void _gcWalk(Type &type);
		void _gcWalk(Value *i);
		void _gcWalk(Context &i);

		void _instantiateGenericValue(Type &type, const GenericArgList &genericArgs) const;
		void _instantiateGenericValue(Value *v, const GenericArgList &genericArgs) const;

		ObjectValue *_newClassInstance(ClassValue *cls);
		ObjectValue *_newGenericClassInstance(ClassValue *cls, GenericArgList &genericArgs);

		void _callFn(Context *context, FnValue *fn);
		VarValue *_addLocalVar(MajorFrame &frame, Type type);
		VarValue *_addLocalReg(MajorFrame &frame);

		bool _findAndDispatchExceptHandler(Context *context) const;

		friend class Value;
		friend class FnValue;
		friend class ObjectValue;
		friend class MemberValue;
		friend class ModuleValue;
		friend class ValueRef<ObjectValue>;

	public:
		/// @brief Runtime flags.
		RuntimeFlags _flags = 0;

		/// @brief Active context on threads.
		std::map<std::thread::id, std::shared_ptr<Context>> activeContexts;

		/// @brief Thread IDs of threads which are executing destructors.
		std::unordered_set<std::thread::id> destructingThreads;

		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		Runtime(RuntimeFlags flags = 0);
		virtual ~Runtime();

		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Value to be instantiated.
		/// @param genericArgs Generic arguments for instantiation.
		/// @return Instantiated value.
		Value *instantiateGenericValue(const Value *v, const std::deque<Type> &genericArgs) const;

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scopeValue Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		Value *resolveRef(RefValue *ref, Value *scopeValue = nullptr) const;

		ValueRef<ModuleValue> loadModule(std::istream &fs, LoadModuleFlags flags);
		ValueRef<ModuleValue> loadModule(const void *buf, size_t size, LoadModuleFlags flags);

		inline RootValue *getRootValue() { return _rootValue; }

		inline void setModuleLocator(ModuleLocatorFn locator) { _moduleLocator = locator; }
		inline ModuleLocatorFn getModuleLocator() { return _moduleLocator; }

		std::string getFullName(const MemberValue *v) const;
		std::string getFullName(const RefValue *v) const;

		std::deque<RefEntry> getFullRef(const MemberValue *v) const;

		/// @brief Get active context on specified thread.
		/// @param id ID of specified thread.
		/// @return Active context on specified thread.
		inline std::shared_ptr<Context> getActiveContext(std::thread::id id = std::this_thread::get_id()) {
			return activeContexts.at(id);
		}

		/// @brief Do a GC cycle.
		void gc();

		std::string mangleName(
			std::string name,
			std::deque<Type> params,
			GenericArgList genericArgs,
			bool isConst) const;

		inline std::string mangleName(
			std::string name,
			std::deque<Type> params,
			bool isConst) const {
			return mangleName(name, params, {}, isConst);
		}
		inline std::string mangleName(
			std::string name,
			std::deque<Type> params,
			GenericArgList genericArgs = {}) const {
			return mangleName(name, params, {}, false);
		}
	};
}

#endif
