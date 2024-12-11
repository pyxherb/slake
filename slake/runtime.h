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
#include "dbg/adapter.h"
#include <slake/util/stream.hh>

namespace slake {
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
		// The runtime is destructing.
		_RT_DEINITING = 0x20000000,
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
			std::pmr::unordered_map<std::pmr::string, Type> mappedGenericArgs;

			SLAKE_FORCEINLINE GenericInstantiationContext(std::pmr::memory_resource *memoryResource) : mappedGenericArgs(memoryResource) {}
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
		size_t _szMemUsedAfterLastGc = 0,
			_szComputedGcLimit = 0;

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
		SLAKE_API [[nodiscard]] InternalExceptionPointer _execIns(ContextObject *context, const Instruction &ins) noexcept;

		struct GCHeaplessWalkContext {
			Object *walkableList = nullptr;

			SLAKE_API void pushObject(Object *object);
		};

		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, Scope *scope);
		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, MethodTable *methodTable);
		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, GenericParamList &genericParamList);
		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, const Type &type);
		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, const Value &i);
		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, Object *i);
		SLAKE_API void _gcWalkHeapless(GCHeaplessWalkContext &context, Context &i);
		SLAKE_API void _gcHeapless();

		SLAKE_API [[nodiscard]] InternalExceptionPointer _instantiateGenericObject(Type &type, GenericInstantiationContext &instantiationContext);
		SLAKE_API [[nodiscard]] InternalExceptionPointer _instantiateGenericObject(Value &value, GenericInstantiationContext &instantiationContext);
		SLAKE_API [[nodiscard]] InternalExceptionPointer _instantiateGenericObject(Object *v, GenericInstantiationContext &instantiationContext);
		SLAKE_API [[nodiscard]] InternalExceptionPointer _instantiateGenericObject(FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext);

		SLAKE_API [[nodiscard]] InternalExceptionPointer _createNewMajorFrame(
			Context *context,
			Object *thisObject,
			const FnOverloadingObject *fn,
			const Value *args,
			uint32_t nArgs) noexcept;

		SLAKE_API [[nodiscard]] InternalExceptionPointer _addLocalVar(MajorFrame *frame, Type type, VarRef &varRefOut) noexcept;

		SLAKE_API uint32_t _findAndDispatchExceptHandler(const Value &curExcept, const MinorFrame &minorFrame) const;

		friend class Object;
		friend class RegularFnOverloadingObject;
		friend class FnObject;
		friend class InstanceObject;
		friend class ModuleObject;

	public:
		/// @brief Runtime flags.
		RuntimeFlags _flags = 0;

		std::list<Object *> createdObjects;

		/// @brief Active contexts of threads.
		std::map<std::thread::id, ContextObject *> activeContexts;

		/// @brief Thread IDs of threads which are executing destructors.
		std::unordered_set<std::thread::id> destructingThreads;

		SLAKE_API Runtime(Runtime &) = delete;
		SLAKE_API Runtime(Runtime &&) = delete;
		SLAKE_API Runtime &operator=(Runtime &) = delete;
		SLAKE_API Runtime &operator=(Runtime &&) = delete;

		SLAKE_API Runtime(std::pmr::memory_resource *upstreamMemoryResource, RuntimeFlags flags = 0);
		SLAKE_API virtual ~Runtime();

		SLAKE_API void invalidateGenericCache(Object *i);

		SLAKE_API [[nodiscard]] InternalExceptionPointer mapGenericParams(const Object *v, GenericInstantiationContext &instantiationContext) const;
		SLAKE_API [[nodiscard]] InternalExceptionPointer mapGenericParams(const FnOverloadingObject *ol, GenericInstantiationContext &instantiationContext) const;
		/// @brief Instantiate an generic value (e.g. generic class, etc).
		/// @param v Object to be instantiated.
		/// @param genericArgs Generic arguments for instantiation.
		/// @return Instantiated value.
		SLAKE_API [[nodiscard]] InternalExceptionPointer instantiateGenericObject(const Object *v, Object *&objectOut, GenericInstantiationContext &instantiationContext);

		SLAKE_API void setGenericCache(const Object *object, const GenericArgList &genericArgs, Object *instantiatedObject);

		/// @brief Resolve a reference and get the referenced value.
		/// @param ref Reference to be resolved.
		/// @param scopeObject Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		SLAKE_API InternalExceptionPointer resolveIdRef(IdRefObject *ref, VarRefContext *varRefContextOut, Object *&objectOut, Object *scopeObject = nullptr);

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

		SLAKE_API void initMethodTableForClass(ClassObject *cls, ClassObject *parentClass);
		SLAKE_API void initObjectLayoutForClass(ClassObject *cls, ClassObject *parentClass);
		SLAKE_API InternalExceptionPointer prepareClassForInstantiation(ClassObject *cls);
		SLAKE_API HostObjectRef<InstanceObject> newClassInstance(ClassObject *cls, NewClassInstanceFlags flags);
		SLAKE_API HostObjectRef<ArrayObject> newArrayInstance(Runtime *rt, const Type &type, size_t length);

		SLAKE_API [[nodiscard]] InternalExceptionPointer execContext(ContextObject *context) noexcept;
		SLAKE_API [[nodiscard]] InternalExceptionPointer execFn(
			const FnOverloadingObject *overloading,
			ContextObject *prevContext,
			Object *thisObject,
			const Value *args,
			uint32_t nArgs,
			HostObjectRef<ContextObject> &contextOut);
	};
}

#endif
