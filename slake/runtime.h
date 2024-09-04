#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <sstream>
#include <thread>
#include <unordered_set>
#include <set>
#include <memory>
#include <memory_resource>

#include "slxfmt.h"
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

	/// @brief Minor frames are created by ENTER instructions and destroyed by
	/// LEAVE instructions.
	struct MinorFrame final {
		std::deque<ExceptionHandler> exceptHandlers;  // Exception handlers

		uint32_t nLocalVars = 0, nRegs = 0;
		size_t stackBase;

		MinorFrame(uint32_t nLocalVars, uint32_t nRegs);
		// Default constructor is required by resize() methods from the
		// containers.
		inline MinorFrame() {
			assert(false);
		}
	};

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		Object *scopeObject = nullptr;						// Scope value.
		const RegularFnOverloadingObject *curFn = nullptr;	// Current function overloading.
		uint32_t curIns = 0;								// Offset of current instruction in function body.
		std::deque<RegularVarObject *> argStack;					// Argument stack.
		std::deque<Value> nextArgStack;						// Argument stack for next call.
		std::deque<Type> nextArgTypes;						// Types of argument stack for next call.
		std::deque<RegularVarObject *> localVars;					// Local variables.
		std::deque<Value> regs;								// Local registers.
		Object *thisObject = nullptr;						// `this' object.
		Value returnValue = nullptr;						// Return value.
		std::deque<MinorFrame> minorFrames;					// Minor frames.
		Value curExcept = nullptr;							// Current exception.

		MajorFrame(Runtime *rt);
		// Default constructor is required by resize() methods from the
		// containers.
		inline MajorFrame() {
			assert(false);
		}

		inline VarRef lload(uint32_t off) {
			if (off >= localVars.size())
				throw InvalidLocalVarIndexError("Invalid local variable index", off);

			return VarRef(localVars.at(off));
		}

		inline VarRef larg(uint32_t off) {
			if (off >= argStack.size())
				throw InvalidArgumentIndexError("Invalid argument index", off);

			return VarRef(argStack.at(off));
		}

		/// @brief Leave current minor frame.
		inline void leave() {
			localVars.resize(minorFrames.back().nLocalVars);
			regs.resize(minorFrames.back().nRegs);
			minorFrames.pop_back();
		}
	};

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Done
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context final {
		std::deque<MajorFrame> majorFrames;	 // Major frame
		ContextFlags flags = 0;				 // Flags
		std::unique_ptr<char[]> dataStack;	 // Data stack
	};

	class SynchronizedCountablePoolResource : public std::pmr::synchronized_pool_resource {
	public:
		size_t szAllocated = 0;

		inline SynchronizedCountablePoolResource() : synchronized_pool_resource() {}
		explicit inline SynchronizedCountablePoolResource(std::pmr::memory_resource *upstream) : synchronized_pool_resource(upstream) {}
		explicit inline SynchronizedCountablePoolResource(const std::pmr::pool_options &opts) : synchronized_pool_resource(opts) {}
		inline SynchronizedCountablePoolResource(const std::pmr::pool_options &opts, std::pmr::memory_resource *upstream) : synchronized_pool_resource(opts, upstream) {}
		SynchronizedCountablePoolResource(const SynchronizedCountablePoolResource &) = delete;

		virtual void *do_allocate(size_t bytes, size_t alignment) override {
			void *p = synchronized_pool_resource::do_allocate(bytes, alignment);

			szAllocated += bytes;

			return p;
		}

		virtual void do_deallocate(void *p, size_t bytes, size_t alignment) override {
			synchronized_pool_resource::do_deallocate(p, bytes, alignment);

			szAllocated -= bytes;
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
		// The runtime is initializing.
		_RT_INITING = 0x80000000;

	using ModuleLocatorFn = std::function<
		std::unique_ptr<std::istream>(Runtime *rt, HostObjectRef<IdRefObject> ref)>;

	using LoadModuleFlags = uint8_t;
	constexpr LoadModuleFlags
		// Do not put the module onto the path where corresponds to the module name.
		LMOD_NOMODNAME = 0x01,
		// Return directly if module with such name exists.
		LMOD_NORELOAD = 0x02,
		// Throw an exception if module that corresponds to the module name exists.
		LMOD_NOCONFLICT = 0x04,
		// Do not import modules.
		LMOD_NOIMPORT = 0x08,
		// Load native members
		LMOD_LOADNATIVE = 0x10;

	class Runtime final {
	public:
		struct GenericInstantiationContext {
			const Object *mappedObject;
			const GenericArgList *genericArgs;
			std::unordered_map<std::pmr::string, Type> mappedGenericArgs;
		};

		SynchronizedCountablePoolResource globalHeapPoolResource;

	private:
		/// @brief Root value of the runtime.
		RootObject *_rootObject;

		struct GenericLookupEntry {
			const Object *originalObject;
			GenericArgList genericArgs;
		};
		mutable std::map<const Object *, GenericLookupEntry> _genericCacheLookupTable;

		using GenericCacheTable =
			std::map<
				GenericArgList,	 // Generic arguments.
				Object *,		 // Cached instantiated value.
				GenericArgListComparator>;

		using GenericCacheDirectory = std::map<
			const Object *,	 // Original uninstantiated generic value.
			GenericCacheTable>;

		/// @brief Cached instances of generic values.
		mutable GenericCacheDirectory _genericCacheDir;

		/// @brief Size of memory allocated for values after last GC cycle.
		size_t _szMemUsedAfterLastGc = 0;

		/// @brief Module locator for importing.
		ModuleLocatorFn _moduleLocator;

		IdRefObject *_loadIdRef(std::istream &fs);
		Value _loadValue(std::istream &fs);
		Type _loadType(std::istream &fs);
		GenericParam _loadGenericParam(std::istream &fs);
		void _loadScope(ModuleObject *mod, std::istream &fs, LoadModuleFlags loadModuleFlags);

		/// @brief Execute a single instruction.
		/// @param context Context for execution.
		/// @param ins Instruction to be executed.
		void _execIns(Context *context, Instruction ins);

		void _gcWalk(Scope *scope);
		void _gcWalk(MethodTable *methodTable);
		void _gcWalk(GenericParamList &genericParamList);
		void _gcWalk(const Type &type);
		void _gcWalk(const Value &i);
		void _gcWalk(Object *i);
		void _gcWalk(Context &i);

		void _instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext) const;
		void _instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext) const;
		void _instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext) const;
		void _instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const;

		RegularVarObject *_addLocalVar(MajorFrame &frame, Type type);
		void _addLocalReg(MajorFrame &frame);

		uint32_t _findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame) const;

		friend class Object;
		friend class RegularFnOverloadingObject;
		friend class FnObject;
		friend class InstanceObject;
		friend class ModuleObject;

	public:
		/// @brief Runtime flags.
		RuntimeFlags _flags = 0;

		std::set<Object *> createdObjects;

		/// @brief Active contexts of threads.
		std::map<std::thread::id, std::shared_ptr<Context>> activeContexts;

		/// @brief Thread IDs of threads which are executing destructors.
		std::unordered_set<std::thread::id> destructingThreads;

		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		Runtime(RuntimeFlags flags = 0);
		virtual ~Runtime();

		inline void invalidateGenericCache(Object *i) {
			if (_genericCacheLookupTable.count(i)) {
				// Remove the value from generic cache if it is unreachable.
				auto &lookupEntry = _genericCacheLookupTable.at(i);

				auto &table = _genericCacheDir.at(lookupEntry.originalObject);
				table.erase(lookupEntry.genericArgs);

				if (!table.size())
					_genericCacheLookupTable.erase(lookupEntry.originalObject);

				_genericCacheLookupTable.erase(i);
			}
		}

		void mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const;
		void mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const;
		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Object to be instantiated.
		/// @param genericArgs Generic arguments for instantiation.
		/// @return Instantiated value.
		Object *instantiateGenericObject(const Object *v, GenericInstantiationContext &instantiationContext) const;

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scopeObject Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		Object *resolveIdRef(IdRefObject *ref, Object *scopeObject = nullptr) const;

		HostObjectRef<ModuleObject> loadModule(std::istream &fs, LoadModuleFlags flags);
		HostObjectRef<ModuleObject> loadModule(const void *buf, size_t size, LoadModuleFlags flags);

		inline RootObject *getRootObject() { return _rootObject; }

		inline void setModuleLocator(ModuleLocatorFn locator) { _moduleLocator = locator; }
		inline ModuleLocatorFn getModuleLocator() { return _moduleLocator; }

		std::string getFullName(const MemberObject *v) const;
		std::string getFullName(const IdRefObject *v) const;

		std::deque<IdRefEntry> getFullRef(const MemberObject *v) const;

		/// @brief Do a GC cycle.
		void gc();

		InstanceObject *newClassInstance(ClassObject *cls);
		HostObjectRef<ArrayObject> newArrayInstance(Runtime *rt, const Type &type, size_t length);
	};
}

#endif
