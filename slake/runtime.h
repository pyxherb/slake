#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <thread>
#include <memory>

#include "slxfmt.h"
#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "object.h"
#include "plat.h"
#include <peff/containers/map.h>
#include <peff/base/deallocable.h>
#include <peff/advutils/buffer_alloc.h>

namespace slake {
	class CountablePoolAlloc : public peff::Alloc {
	protected:
		std::atomic_size_t refCount = 0;

	public:
		Runtime *runtime;

		peff::RcObjectPtr<peff::Alloc> upstream;
		std::atomic_size_t szAllocated = 0;

		SLAKE_API CountablePoolAlloc(Runtime *runtime, peff::Alloc *upstream);

		SLAKE_API virtual size_t incRef(size_t globalRc) noexcept override;
		SLAKE_API virtual size_t decRef(size_t globalRc) noexcept override;

		SLAKE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLAKE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept override;
		SLAKE_API virtual void release(void *p, size_t size, size_t alignment) noexcept override;

		SLAKE_API virtual bool isReplaceable(const peff::Alloc *rhs) const noexcept override;

		SLAKE_API virtual peff::UUID getTypeId() const noexcept override;
		SLAKE_API virtual void onRefZero() noexcept;
	};

	class GenerationalPoolAlloc : public peff::Alloc {
	protected:
		std::atomic_size_t refCount = 0;
		friend class Runtime;

	public:
		Runtime *runtime;

#ifndef _NDEBUG
		peff::Set<size_t> recordedRefPoints;
#endif

		peff::RcObjectPtr<peff::Alloc> upstream;
		std::atomic_size_t szAllocated = 0;

		SLAKE_API GenerationalPoolAlloc(Runtime *runtime, peff::Alloc *upstream);

		SLAKE_API virtual size_t incRef(size_t globalRc) noexcept override;
		SLAKE_API virtual size_t decRef(size_t globalRc) noexcept override;

		SLAKE_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		SLAKE_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t newSize, size_t newAlignment) noexcept override;
		SLAKE_API virtual void release(void *p, size_t size, size_t alignment) noexcept override;

		SLAKE_API virtual bool isReplaceable(const peff::Alloc *rhs) const noexcept override;

		SLAKE_API virtual peff::UUID getTypeId() const noexcept override;
		SLAKE_API virtual void onRefZero() noexcept;
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
		// The runtime is destructing.
		_RT_DEINITING = 0x20000000,
		// The runtime is in a GC cycle.
		_RT_INGC = 0x40000000,
		// The runtime is initializing.
		_RT_INITING = 0x80000000;

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
		Mutex accessMutex;
		InstanceObject *unwalkedInstanceList = nullptr;
		InstanceObject *destructibleList = nullptr;
		InstanceObject *typeDefList = nullptr;
		Object *unwalkedList = nullptr;
		Object *walkedList = nullptr;

	public:
		static SLAKE_API void pushObject(GCWalkContext *context, Object *object);
		static SLAKE_API void removeFromUnwalkedList(Object *v);
		static SLAKE_API void removeFromDestructibleList(Object *v);

		SLAKE_API bool isWalkableListEmpty();
		SLAKE_API Object *getWalkableList();
		SLAKE_API void pushWalkable(Object *walkableObject);

		SLAKE_API Object *getUnwalkedList(bool clearList);
		SLAKE_API void pushUnwalked(Object *walkableObject);
		SLAKE_API void updateUnwalkedList(Object *deletedObject);

		SLAKE_API Object *getWalkedList();

		SLAKE_API InstanceObject *getDestructibleList();
		SLAKE_API void pushDestructible(InstanceObject *v);

		SLAKE_API void reset();
	};

	class Runtime final {
	public:
		class GenericInstantiationContext;

		struct GenericInstantiationDispatcher;

		mutable CountablePoolAlloc fixedAlloc;
		mutable GenerationalPoolAlloc youngAlloc;
		mutable GenerationalPoolAlloc persistentAlloc;

	private:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		/// @brief Root value of the runtime.
		ModuleObject *_rootObject;

		struct GenericLookupEntry {
			MemberObject *originalObject;
			peff::DynArray<Value> genericArgs;
		};
		mutable peff::Map<const MemberObject *, GenericLookupEntry> _genericCacheLookupTable;

		using GenericCacheTable =
			peff::Map<
				peff::DynArray<Value>,	// Generic arguments.
				MemberObject *,	 // Cached instantiated value.
				GenericArgListComparator,
				true>;

		using GenericCacheDirectory = peff::Map<
			MemberObject *,  // Original uninstantiated generic value.
			GenericCacheTable>;

		/// @brief Cached instances of generic values.
		mutable GenericCacheDirectory _genericCacheDir;

		/// @brief Size of memory allocated for values after last GC cycle.
		size_t _szMemUsedAfterLastGc = 0,
			   _szComputedGcLimit = 0;

		UncaughtExceptionHandler _uncaughtExceptionHandler = nullptr;

		struct LoaderContext {
			std::istream &fs;
			Object *ownerObject;
			bool isInGenericScope;
		};

		/// @brief Execute a single instruction.
		/// @param context Context for execution.
		/// @param ins Instruction to be executed.
		[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _execIns(ContextObject *context, MajorFrame *curMajorFrame, const Instruction &ins, bool &isContextChangedOut) noexcept;

	public:
		Object *youngObjectList = nullptr, *persistentObjectList = nullptr;
		size_t nYoungObjects = 0, nPersistentObjects = 0;

		Object *instanceObjectList = nullptr;
		Object *contextObjectList = nullptr;
		Object *classObjectList = nullptr;

		peff::Set<TypeDefObject *, TypeDefComparator, true> typeDefs;

		SLAKE_API TypeDefObject *getEqualTypeDef(TypeDefObject *typeDef) const noexcept;
		SLAKE_API void unregisterTypeDef(TypeDefObject *typeDef) noexcept;
		SLAKE_API InternalExceptionPointer registerTypeDef(TypeDefObject *typeDef) noexcept;

	private:
		SLAKE_API void _gcWalk(GCWalkContext *context, MethodTable *methodTable);
		SLAKE_API void _gcWalk(GCWalkContext *context, GenericParamList &genericParamList);
		SLAKE_API void _gcWalk(GCWalkContext *context, const TypeRef &type);
		SLAKE_API void _gcWalk(GCWalkContext *context, const Value &i);
		SLAKE_API void _gcWalk(GCWalkContext *context, Object *i);
		SLAKE_API void _gcWalk(GCWalkContext *context, char *dataStack, size_t stackSize, MajorFrame *majorFrame);
		SLAKE_API void _gcWalk(GCWalkContext *context, Context &i);
		SLAKE_API void _gcSerial(Object *&objectList, Object *&endObjectOut, size_t &nObjects, ObjectGeneration newGeneration);

		std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>> parallelGcThread;

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
			Cond activeCond, doneCond;
			ParallelGcThreadState threadState = ParallelGcThreadState::Alive;

			SLAKE_API ParallelGcThreadRunnable(Runtime *runtime);
			SLAKE_API virtual void run() override;

			SLAKE_API void dealloc();
		};

		std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>> parallelGcThreadRunnable;

		SLAKE_API bool _allocParallelGcResources();
		SLAKE_API void _releaseParallelGcResources();

		SLAKE_API void _gcParallelHeapless(Object *&objectList, Object *&endObjectOut, size_t &nObjects, ObjectGeneration newGeneration);

		SLAKE_API void _destructDestructibleObjects(InstanceObject *destructibleList);

		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateModuleFields(GenericInstantiationDispatcher &dispatcher, ModuleObject *mod, GenericInstantiationContext *instantiationContext);

		[[nodiscard]] SLAKE_API InternalExceptionPointer _mapGenericParams(const Object *v, GenericInstantiationContext *instantiationContext) const;
		[[nodiscard]] SLAKE_API InternalExceptionPointer _mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext *instantiationContext) const;

		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Reference dest, Value value, GenericInstantiationContext *instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, TypeRef &type, GenericInstantiationContext *instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Value &value, GenericInstantiationContext *instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, Object *v, GenericInstantiationContext *instantiationContext);
		[[nodiscard]] SLAKE_API InternalExceptionPointer _instantiateGenericObject(GenericInstantiationDispatcher &dispatcher, FnOverloadingObject *ol, GenericInstantiationContext *instantiationContext);

		SLAKE_API InternalExceptionPointer _findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame, uint32_t &offsetOut) const;

		friend class Object;
		friend class RegularFnOverloadingObject;
		friend class FnObject;
		friend class InstanceObject;
		friend class ModuleObject;

	public:
		[[nodiscard]] SLAKE_API InternalExceptionPointer _addLocalVar(Context *context, MajorFrame *frame, TypeRef type, Reference &objectRefOut) noexcept;
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

		class GenericInstantiationContext final {
		public:
			std::atomic_size_t refCount = 0;
			peff::RcObjectPtr<peff::Alloc> selfAllocator;
			const Object *mappedObject;
			const peff::DynArray<Value> *genericArgs;
			peff::HashMap<std::string_view, Value> mappedGenericArgs;

			SLAKE_FORCEINLINE GenericInstantiationContext(peff::Alloc *selfAllocator, peff::Alloc *resourceAllocator) : selfAllocator(selfAllocator), mappedGenericArgs(resourceAllocator) {}
			SLAKE_FORCEINLINE GenericInstantiationContext(
				peff::Alloc *selfAllocator,
				const Object *mappedObject,
				const peff::DynArray<Value> *genericArgs,
				peff::HashMap<std::string_view, Value> &&mappedGenericArgs)
				: selfAllocator(selfAllocator),
				  mappedObject(mappedObject),
				  genericArgs(genericArgs),
				  mappedGenericArgs(std::move(mappedGenericArgs)) {
			}

			SLAKE_FORCEINLINE size_t incRef(size_t globalRc) noexcept {
				SLAKE_REFERENCED_PARAM(globalRc);
				return ++refCount;
			}

			SLAKE_FORCEINLINE size_t decRef(size_t globalRc) noexcept {
				SLAKE_REFERENCED_PARAM(globalRc);
				if (!--refCount) {
					onRefZero();
					return 0;
				}

				return refCount;
			}

			SLAKE_FORCEINLINE void onRefZero() noexcept {
				peff::destroyAndRelease<GenericInstantiationContext>(selfAllocator.get(), this, alignof(GenericInstantiationContext));
			}
		};

		SLAKE_API void invalidateGenericCache(MemberObject *i);

		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Object to be instantiated.
		/// @param genericArgs Generic arguments for instantiation.
		/// @return Instantiated value.
		[[nodiscard]] SLAKE_API InternalExceptionPointer instantiateGenericObject(MemberObject *object, MemberObject *&objectOut, GenericInstantiationContext *instantiationContext);

		SLAKE_API InternalExceptionPointer setGenericCache(MemberObject *object, const peff::DynArray<Value> &genericArgs, MemberObject *instantiatedObject);

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scopeObject Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		SLAKE_API InternalExceptionPointer resolveIdRef(IdRefObject *ref, Reference &objectRefOut, Object *scopeObject = nullptr);

		SLAKE_API static void addSameKindObjectToList(Object **list, Object *object);
		SLAKE_API static void removeSameKindObjectToList(Object **list, Object *object);
		[[nodiscard]] SLAKE_API bool addObject(Object *object);
		SLAKE_FORCEINLINE peff::Alloc *getFixedAlloc() const {
			return &fixedAlloc;
		}
		SLAKE_API peff::Alloc *getCurGenAlloc();

		SLAKE_API HostObjectRef<ModuleObject> loadModule(const void *buf, size_t size, LoadModuleFlags flags);

		SLAKE_FORCEINLINE ModuleObject *getRootObject() { return _rootObject; }

		[[nodiscard]] SLAKE_API bool getFullRef(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const;

		/// @brief Run a cycle of GC.
		SLAKE_API void gc();

		[[nodiscard]] SLAKE_API InternalExceptionPointer initMethodTableForClass(ClassObject *cls, ClassObject *parentClass);
		[[nodiscard]] SLAKE_API InternalExceptionPointer initObjectLayoutForModule(ModuleObject *mod, ObjectLayout *objectLayout);
		[[nodiscard]] SLAKE_API InternalExceptionPointer initObjectLayoutForClass(ClassObject *cls, ClassObject *parentClass);
		[[nodiscard]] SLAKE_API InternalExceptionPointer initObjectLayoutForStruct(StructObject *s);
		[[nodiscard]] SLAKE_API InternalExceptionPointer prepareClassForInstantiation(ClassObject *cls);
		[[nodiscard]] SLAKE_API InternalExceptionPointer prepareStructForInstantiation(StructObject *cls);
		SLAKE_API HostObjectRef<InstanceObject> newClassInstance(ClassObject *cls, NewClassInstanceFlags flags);
		SLAKE_API HostObjectRef<ArrayObject> newArrayInstance(Runtime *rt, const TypeRef &type, size_t length);

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
			ContextObject *context,
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
			ContextObject *context,
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

		SLAKE_API static char *calcCoroutineLocalVarRefStackBasePtr(const CoroutineLocalVarRef &localVarRef);
		SLAKE_API static char *calcLocalVarRefStackBasePtr(const LocalVarRef &localVarRef);
		SLAKE_API static char *calcLocalVarRefStackRawDataPtr(char *p);

		[[nodiscard]] SLAKE_API TypeRef typeofVar(const Reference &entityRef) const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer readVar(const Reference &entityRef, Value &valueOut) const noexcept;
		SLAKE_FORCEINLINE Value readVarUnsafe(const Reference& entityRef) const noexcept {
			Value v;
			readVar(entityRef, v).unwrap();
			return v;
		}
		SLAKE_API void readStructData(char *dest, const StructRef &structRef) const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer writeVar(const Reference &entityRef, const Value &value) const noexcept;
		SLAKE_FORCEINLINE void writeVarUnsafe(const Reference& entityRef, const Value& value) const noexcept {
			writeVar(entityRef, value).unwrap();
		}

		SLAKE_API size_t sizeofType(const TypeRef &type) const;
		SLAKE_API size_t alignofType(const TypeRef &type) const;
		SLAKE_API Value defaultValueOf(const TypeRef &type) const;
		SLAKE_API InternalExceptionPointer loadDeferredCustomTypeDef(CustomTypeDefObject *customTypeDef);

		[[nodiscard]] SLAKE_API static bool constructAt(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags = 0);
		[[nodiscard]] SLAKE_API static Runtime *alloc(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags = 0);

		SLAKE_API void dealloc() noexcept;
	};
}

#endif
