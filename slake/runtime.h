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
#include <slake/util/stream.hh>

namespace slake {
	struct ExceptionHandler final {
		Type type;
		uint32_t off;
	};

	/// @brief Minor frames are created by ENTER instructions and destroyed by
	/// LEAVE instructions.
	struct MinorFrame final {
		std::pmr::vector<ExceptionHandler> exceptHandlers;  // Exception handlers

		uint32_t nLocalVars = 0, nRegs = 0;
		size_t stackBase = 0;

		SLAKE_API MinorFrame(
			Runtime *rt,
			uint32_t nLocalVars,
			uint32_t nRegs,
			size_t stackBase);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MinorFrame() {
			abort();
		}
	};

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		Context *context = nullptr;		// Context
		Object *scopeObject = nullptr;	// Scope value.

		const RegularFnOverloadingObject *curFn = nullptr;	// Current function overloading.
		uint32_t curIns = 0;								// Offset of current instruction in function body.

		std::pmr::vector<RegularVarObject *> argStack;  // Argument stack.

		std::pmr::vector<Value> nextArgStack;  // Argument stack for next call.
		std::pmr::vector<Type> nextArgTypes;   // Types of argument stack for next call.

		std::pmr::vector<LocalVarRecord> localVarRecords;  // Local variable records.
		LocalVarAccessorVarObject *localVarAccessor;  // Local variable accessor.

		std::pmr::vector<Value> regs;  // Local registers.

		Object *thisObject = nullptr;  // `this' object.

		Value returnValue = nullptr;  // Return value.

		std::pmr::vector<MinorFrame> minorFrames;	 // Minor frames.

		Value curExcept = nullptr;	// Current exception.

		SLAKE_API MajorFrame(Runtime *rt, Context *context);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MajorFrame() {
			abort();
		}

		SLAKE_API VarRef lload(uint32_t off);

		SLAKE_API VarRef larg(uint32_t off);

		/// @brief Leave current minor frame.
		SLAKE_API void leave();
	};

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Done
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context {
		std::vector<std::unique_ptr<MajorFrame>> majorFrames;  // Major frame
		ContextFlags flags = 0;								   // Flags
		char *dataStack = nullptr;							   // Data stack
		size_t stackTop = 0;								   // Stack top

		SLAKE_API char *stackAlloc(size_t size);

		SLAKE_API Context();

		SLAKE_API ~Context();
	};

	class CountablePoolResource : public std::pmr::memory_resource {
	public:
		std::pmr::memory_resource *upstream;
		size_t szAllocated = 0;

		SLAKE_API CountablePoolResource(std::pmr::memory_resource *upstream);
		SLAKE_API CountablePoolResource(const CountablePoolResource &) = delete;

		SLAKE_API virtual void *do_allocate(size_t bytes, size_t alignment) override;
		SLAKE_API virtual void do_deallocate(void *p, size_t bytes, size_t alignment) override;
		SLAKE_API virtual bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override;
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

	using NewClassInstanceFlags = uint8_t;
	constexpr static NewClassInstanceFlags
		_NEWCLSINST_PARENT = 0x80;

	class Runtime final {
	public:
		struct GenericInstantiationContext {
			const Object *mappedObject;
			const GenericArgList *genericArgs;
			std::unordered_map<std::pmr::string, Type> mappedGenericArgs;
		};

		mutable CountablePoolResource globalHeapPoolResource;

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

		struct LoaderContext {
			std::istream &fs;
			Object *ownerObject;
		};

		SLAKE_API HostObjectRef<IdRefObject> _loadIdRef(LoaderContext &context, HostRefHolder &holder);
		SLAKE_API Value _loadValue(LoaderContext &context, HostRefHolder &holder);
		SLAKE_API Type _loadType(LoaderContext &context, HostRefHolder &holder);
		SLAKE_API GenericParam _loadGenericParam(LoaderContext &context, HostRefHolder &holder);
		SLAKE_API void _loadScope(LoaderContext &context,
			HostObjectRef<ModuleObject> mod,
			LoadModuleFlags loadModuleFlags,
			HostRefHolder &holder);

		/// @brief Execute a single instruction.
		/// @param context Context for execution.
		/// @param ins Instruction to be executed.
		SLAKE_API void _execIns(Context *context, Instruction ins);

		SLAKE_API void _gcWalk(Scope *scope);
		SLAKE_API void _gcWalk(MethodTable *methodTable);
		SLAKE_API void _gcWalk(GenericParamList &genericParamList);
		SLAKE_API void _gcWalk(const Type &type);
		SLAKE_API void _gcWalk(const Value &i);
		SLAKE_API void _gcWalk(Object *i);
		SLAKE_API void _gcWalk(Context &i);

		SLAKE_API void _instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext) const;
		SLAKE_API void _instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext) const;
		SLAKE_API void _instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext) const;
		SLAKE_API void _instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const;

		SLAKE_API VarRef _addLocalVar(MajorFrame *frame, Type type);
		SLAKE_API void _addLocalReg(MajorFrame *frame);

		SLAKE_API uint32_t _findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame) const;

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

		SLAKE_API Runtime(Runtime &) = delete;
		SLAKE_API Runtime(Runtime &&) = delete;
		SLAKE_API Runtime &operator=(Runtime &) = delete;
		SLAKE_API Runtime &operator=(Runtime &&) = delete;

		SLAKE_API Runtime(std::pmr::memory_resource *upstreamMemoryResource, RuntimeFlags flags = 0);
		SLAKE_API virtual ~Runtime();

		SLAKE_API void invalidateGenericCache(Object *i);

		SLAKE_API void mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const;
		SLAKE_API void mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const;
		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Object to be instantiated.
		/// @param genericArgs Generic arguments for instantiation.
		/// @return Instantiated value.
		SLAKE_API Object *instantiateGenericObject(const Object *v, GenericInstantiationContext &instantiationContext) const;

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scopeObject Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		SLAKE_API Object *resolveIdRef(IdRefObject *ref, VarRefContext *varRefContextOut, Object *scopeObject = nullptr) const;

		SLAKE_API HostObjectRef<ModuleObject> loadModule(std::istream &fs, LoadModuleFlags flags);
		SLAKE_API HostObjectRef<ModuleObject> loadModule(const void *buf, size_t size, LoadModuleFlags flags);

		SLAKE_FORCEINLINE RootObject *getRootObject() { return _rootObject; }

		SLAKE_FORCEINLINE void setModuleLocator(ModuleLocatorFn locator) { _moduleLocator = locator; }
		SLAKE_FORCEINLINE ModuleLocatorFn getModuleLocator() { return _moduleLocator; }

		SLAKE_API std::string getFullName(const MemberObject *v) const;
		SLAKE_API std::string getFullName(const IdRefObject *v) const;

		SLAKE_API std::deque<IdRefEntry> getFullRef(const MemberObject *v) const;

		/// @brief Do a GC cycle.
		SLAKE_API void gc();

		SLAKE_API HostObjectRef<InstanceObject> newClassInstance(ClassObject *cls, NewClassInstanceFlags flags);
		SLAKE_API HostObjectRef<ArrayObject> newArrayInstance(Runtime *rt, const Type &type, size_t length);
	};
}

#endif
