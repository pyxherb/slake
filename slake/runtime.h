#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <sstream>
#include <thread>
#include <unordered_set>
#include <set>
#include <list>
#include <memory>
#include <memory_resource>

#include "slxfmt.h"
#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "object.h"
#include <slake/util/stream.hh>
#include "plat.h"
#include <peff/containers/map.h>
#include <peff/base/deallocable.h>

namespace slake {
	class CountablePoolAlloc : public peff::Alloc {
	public:
		peff::RcObjectPtr<peff::Alloc> upstream;
		size_t szAllocated = 0;

		SLAKE_API CountablePoolAlloc(peff::Alloc *upstream);

		SLAKE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLAKE_API virtual void release(void *p, size_t size, size_t alignment) noexcept override;

		SLAKE_API virtual peff::Alloc *getDefaultAlloc() const noexcept override;
		SLAKE_API virtual void onRefZero() noexcept override;
	};
	SLAKE_API extern CountablePoolAlloc g_countablePoolDefaultAlloc;

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
		// The runtime is destructing.
		_RT_DEINITING = 0x20000000,
		// The runtime is in a GC cycle.
		_RT_INGC = 0x40000000,
		// The runtime is initializing.
		_RT_INITING = 0x80000000;

	typedef std::unique_ptr<std::istream> (*ModuleLocatorFn)(Runtime *rt, const peff::DynArray<IdRefEntry> &ref);

	struct SecurityPolicy {
		bool allowUnsafe;
	};

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
		LMOD_LOADNATIVE = 0x10,
		// Load as implicitly loaded.
		LMOD_IMPLICIT = 0x80;

	using NewClassInstanceFlags = uint8_t;
	constexpr static NewClassInstanceFlags
		_NEWCLSINST_PARENT = 0x80;

	typedef void (*UncaughtExceptionHandler)(InternalExceptionPointer &&exception);

	struct GCWalkContext {
	private:
		Object *walkableList = nullptr;
		std::mutex accessMutex;
		InstanceObject *unwalkedInstanceList = nullptr;
		InstanceObject *destructibleList = nullptr;
		Object *unwalkedList = nullptr;

	public:
		static SLAKE_API void pushObject(Object *object);
		static SLAKE_API void removeFromUnwalkedList(Object *v);
		static SLAKE_API void removeFromDestructibleList(Object *v);

		SLAKE_API bool isWalkableListEmpty();
		SLAKE_API Object *getWalkableList();
		SLAKE_API void pushWalkable(Object *walkableObject);

		SLAKE_API Object *getUnwalkedList();
		SLAKE_API void pushUnwalked(Object *walkableObject);

		SLAKE_API InstanceObject *getDestructibleList();
		SLAKE_API void pushDestructible(InstanceObject *v);

		SLAKE_API void reset();
	};

	class Runtime final {
	public:
		struct GenericInstantiationContext {
			const Object *mappedObject;
			const GenericArgList *genericArgs;
			peff::HashMap<peff::String, Type> mappedGenericArgs;

			SLAKE_FORCEINLINE GenericInstantiationContext(peff::Alloc *selfAllocator) : mappedGenericArgs(selfAllocator) {}
			SLAKE_FORCEINLINE GenericInstantiationContext(
				const Object *mappedObject,
				const GenericArgList *genericArgs,
				peff::HashMap<peff::String, Type> &&mappedGenericArgs)
				: mappedObject(mappedObject),
				  genericArgs(genericArgs),
				  mappedGenericArgs(std::move(mappedGenericArgs)) {
			}
		};

		mutable CountablePoolAlloc globalHeapPoolAlloc;

	private:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		/// @brief Root value of the runtime.
		ModuleObject *_rootObject;

		struct GenericLookupEntry {
			const Object *originalObject;
			GenericArgList genericArgs;
		};
		mutable peff::Map<const Object *, GenericLookupEntry> _genericCacheLookupTable;

		using GenericCacheTable =
			peff::Map<
				GenericArgList,	 // Generic arguments.
				Object *,		 // Cached instantiated value.
				GenericArgListComparator>;

		using GenericCacheDirectory = peff::Map<
			const Object *,	 // Original uninstantiated generic value.
			GenericCacheTable>;

		/// @brief Cached instances of generic values.
		mutable GenericCacheDirectory _genericCacheDir;

		/// @brief Size of memory allocated for values after last GC cycle.
		size_t _szMemUsedAfterLastGc = 0,
			   _szComputedGcLimit = 0;

		/// @brief Module locator for importing.
		ModuleLocatorFn _moduleLocator;

		UncaughtExceptionHandler _uncaughtExceptionHandler = nullptr;

		struct LoaderContext {
			std::istream &fs;
			Object *ownerObject;
			bool isInGenericScope;
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
		[[nodiscard]] SLAKE_API InternalExceptionPointer _execIns(ContextObject *context, MajorFrame *curMajorFrame, const Instruction &ins, bool &isContextChangedOut) noexcept;

		SLAKE_API void _gcWalk(MethodTable *methodTable);
		SLAKE_API void _gcWalk(GenericParamList &genericParamList);
		SLAKE_API void _gcWalk(const Type &type);
		SLAKE_API void _gcWalk(const Value &i);
		SLAKE_API void _gcWalk(Object *i);
		SLAKE_API void _gcWalk(char *dataStack, MajorFrame *majorFrame);
		SLAKE_API void _gcWalk(Context &i);
		SLAKE_API void _gc();

		size_t nMaxGcThreads = 8;
		peff::DynArray<std::unique_ptr<Thread, util::DeallocableDeleter<Thread>>> parallelGcThreads;

		enum class ParallelGcThreadState : uint8_t {
			Alive = 0,
			NotifyTermination,
			Terminated
		};

		class ParallelGcThreadRunnable : public Runnable {
		public:
			Runtime *runtime;
			GCWalkContext context;
			bool isActive = false, isDone = false;
			ParallelGcThreadState threadState = ParallelGcThreadState::Alive;

			SLAKE_API ParallelGcThreadRunnable(Runtime *runtime);
			SLAKE_API virtual void run() override;

			SLAKE_API void dealloc();
		};

		peff::DynArray<std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>>> parallelGcThreadRunnables;

		SLAKE_API bool _allocParallelGcResources();
		SLAKE_API void _releaseParallelGcResources();

		SLAKE_API void _gcParallelHeapless();

		SLAKE_API void _destructDestructibleObjects(InstanceObject *destructibleList);

		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext);

		SLAKE_API uint32_t _findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame) const;

		friend class Object;
		friend class RegularFnOverloadingObject;
		friend class FnObject;
		friend class InstanceObject;
		friend class ModuleObject;

	public:
		[[nodiscard]] SLAKE_API InternalExceptionPointer _addLocalVar(Context *context, MajorFrame *frame, Type type, EntityRef &objectRefOut) noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer _fillArgs(
			MajorFrame *newMajorFrame,
			const FnOverloadingObject *fn,
			const Value *args,
			uint32_t nArgs,
			HostRefHolder &holder);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _createNewCoroutineMajorFrame(
			Context *context,
			CoroutineObject *coroutine,
			uint32_t returnValueOut) noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer _createNewMajorFrame(
			Context *context,
			Object *thisObject,
			const FnOverloadingObject *fn,
			const Value *args,
			uint32_t nArgs,
			uint32_t returnValueOut) noexcept;

		/// @brief Runtime flags.
		RuntimeFlags _flags = 0;

		peff::Set<Object *> createdObjects;

		/// @brief Active contexts of threads.
		std::map<std::thread::id, ContextObject *> activeContexts;

		/// @brief Thread IDs of threads which are executing destructors.
		peff::Map<NativeThreadHandle, ExecutionRunnable *> managedThreadRunnables;

		SLAKE_API Runtime(Runtime &) = delete;
		SLAKE_API Runtime(Runtime &&) = delete;
		SLAKE_API Runtime &operator=(Runtime &) = delete;
		SLAKE_API Runtime &operator=(Runtime &&) = delete;

		SLAKE_API Runtime(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags = 0);
		SLAKE_API virtual ~Runtime();

		SLAKE_API void invalidateGenericCache(Object *i);

		[[nodiscard]] SLAKE_API InternalExceptionPointer instantiateModuleFields(ModuleObject *mod, GenericInstantiationContext &instantiationContext);

		[[nodiscard]] SLAKE_API InternalExceptionPointer mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const;
		[[nodiscard]] SLAKE_API InternalExceptionPointer mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const;
		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Object to be instantiated.
		/// @param genericArgs Generic arguments for instantiation.
		/// @return Instantiated value.
		[[nodiscard]] SLAKE_API InternalExceptionPointer instantiateGenericObject(const Object *v, Object *&objectOut, GenericInstantiationContext &instantiationContext);

		SLAKE_API InternalExceptionPointer setGenericCache(const Object *object, const GenericArgList &genericArgs, Object *instantiatedObject);

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scopeObject Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		SLAKE_API InternalExceptionPointer resolveIdRef(IdRefObject *ref, EntityRef &objectRefOut, Object *scopeObject = nullptr);

		SLAKE_API HostObjectRef<ModuleObject> loadModule(const void *buf, size_t size, LoadModuleFlags flags);

		SLAKE_FORCEINLINE ModuleObject *getRootObject() { return _rootObject; }

		SLAKE_FORCEINLINE void setModuleLocator(ModuleLocatorFn locator) { _moduleLocator = locator; }
		SLAKE_FORCEINLINE ModuleLocatorFn getModuleLocator() { return _moduleLocator; }

		[[nodiscard]] SLAKE_API bool getFullRef(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const;

		/// @brief Run a cycle of GC.
		SLAKE_API void gc();

		[[nodiscard]] SLAKE_API InternalExceptionPointer initMethodTableForClass(ClassObject *cls, ClassObject *parentClass);
		[[nodiscard]] SLAKE_API InternalExceptionPointer initObjectLayoutForClass(ClassObject *cls, ClassObject *parentClass);
		[[nodiscard]] SLAKE_API InternalExceptionPointer prepareClassForInstantiation(ClassObject *cls);
		SLAKE_API HostObjectRef<InstanceObject> newClassInstance(ClassObject *cls, NewClassInstanceFlags flags);
		SLAKE_API HostObjectRef<ArrayObject> newArrayInstance(Runtime *rt, const Type &type, size_t length);

		[[nodiscard]] SLAKE_API InternalExceptionPointer execContext(ContextObject *context) noexcept;
		/// @brief Execute a function on current thread.
		/// @param overloading Function overloading to be executed.
		/// @param prevContext Previous context for execution.
		/// @param thisObject This object for execution.
		/// @param args Argument that will be passed to the function.
		/// @param nArgs Number of arguments.
		/// @param contextOut Where to receive the execution context.
		/// @param nativeStackBaseCurrentPtr Approximate value of the initial stack pointer, used for platforms that do not support stack information features.
		/// @param nativeStackSize Approximate value of the native stack size, used for platforms that do not support stack information features.
		/// @note `nativeBaseCurrentPtr` and `nativeStackSize` is used for native
		/// stack size estimation on platforms that do not support stack information features,
		/// for platform with stack information features, the arguments are ignored.
		///
		/// @return
		[[nodiscard]] SLAKE_API InternalExceptionPointer execFn(
			const FnOverloadingObject *overloading,
			ContextObject *prevContext,
			Object *thisObject,
			const Value *args,
			uint32_t nArgs,
			Value &valueOut,
			void *nativeStackBaseCurrentPtr = nullptr,
			size_t nativeStackSize = 0);
		[[nodiscard]] SLAKE_API InternalExceptionPointer execFnInAotFn(
			const FnOverloadingObject *overloading,
			ContextObject *context,
			Object *thisObject,
			const Value *args,
			uint32_t nArgs,
			void *nativeStackBaseCurrentPtr = nullptr,
			size_t nativeStackSize = 0);
		[[nodiscard]] SLAKE_API InternalExceptionPointer execFnWithSeparatedExecutionThread(
			const FnOverloadingObject *overloading,
			ContextObject *prevContext,
			Object *thisObject,
			const Value *args,
			uint32_t nArgs,
			HostObjectRef<ContextObject> &contextOut);
		[[nodiscard]] SLAKE_API InternalExceptionPointer createCoroutineInstance(
			const FnOverloadingObject *fn,
			Object *thisObject,
			const Value *args,
			uint32_t nArgs,
			HostObjectRef<CoroutineObject> &coroutineOut);
		[[nodiscard]] SLAKE_API InternalExceptionPointer resumeCoroutine(
			ContextObject *context,
			CoroutineObject *coroutine,
			Value &resultOut,
			void *nativeStackBaseCurrentPtr = nullptr,
			size_t nativeStackSize = 0);

		[[nodiscard]] SLAKE_API InternalExceptionPointer tryAccessVar(const EntityRef &entityRef) const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer typeofVar(const EntityRef &entityRef, Type &typeOut) const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer readVar(const EntityRef &entityRef, Value &valueOut) const noexcept;
		[[nodiscard]] SLAKE_API Value readVarUnsafe(const EntityRef &entityRef) const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer writeVar(const EntityRef &entityRef, const Value &value) const noexcept;
		SLAKE_API void writeVarUnsafe(const EntityRef &entityRef, const Value &value) const noexcept;

		SLAKE_API size_t sizeofType(const Type &type);
		SLAKE_API size_t alignofType(const Type &type);
		SLAKE_API Value defaultValueOf(const Type &type);

		[[nodiscard]] SLAKE_API static bool constructAt(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags = 0);
		[[nodiscard]] SLAKE_API static Runtime *alloc(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags = 0);

		[[nodiscard]] SLAKE_API void dealloc() noexcept;
	};
}

#endif
