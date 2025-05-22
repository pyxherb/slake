#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_gcWalk(MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto i = methodTable->methods.begin(); i != methodTable->methods.end(); ++i) {
		GCWalkContext::pushObject(i.value());
	}
	for (auto i : methodTable->destructors) {
		GCWalkContext::pushObject(i);
	}
}

SLAKE_API void Runtime::_gcWalk(GenericParamList &genericParamList) {
	for (auto &i : genericParamList) {
		// i.baseType.loadDeferredType(this);
		if (auto p = i.baseType.resolveCustomType(); p)
			GCWalkContext::pushObject(p);

		for (auto &j : i.interfaces) {
			// j.loadDeferredType(this);
			if (auto p = j.resolveCustomType(); p)
				GCWalkContext::pushObject(p);
		}
	}
}

SLAKE_API void Runtime::_gcWalk(const Type &type) {
	switch (type.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
		case TypeId::String:
			break;
		case TypeId::Instance:
			GCWalkContext::pushObject(type.getCustomTypeExData());
			break;
		case TypeId::GenericArg:
			GCWalkContext::pushObject(type.exData.genericArg.ownerObject);
			GCWalkContext::pushObject(type.exData.genericArg.nameObject);
			break;
		case TypeId::Array:
			if (type.exData.typeDef)
				GCWalkContext::pushObject(type.exData.typeDef);
			break;
		case TypeId::Ref:
			if (type.exData.typeDef)
				GCWalkContext::pushObject(type.exData.typeDef);
			break;
		case TypeId::None:
		case TypeId::Any:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
}

SLAKE_API void Runtime::_gcWalk(const Value &i) {
	bool isWalkableObjectDetected = false;
	switch (i.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
			break;
		case ValueType::EntityRef: {
			const EntityRef &entityRef = i.getEntityRef();

			switch (entityRef.kind) {
				case ObjectRefKind::FieldRef:
					GCWalkContext::pushObject(entityRef.asField.moduleObject);
					break;
				case ObjectRefKind::ArrayElementRef:
					GCWalkContext::pushObject(entityRef.asArray.arrayObject);
					break;
				case ObjectRefKind::ObjectRef:
					GCWalkContext::pushObject(entityRef.asObject.instanceObject);
					break;
				case ObjectRefKind::InstanceFieldRef:
					GCWalkContext::pushObject(entityRef.asObjectField.instanceObject);
					break;
				case ObjectRefKind::LocalVarRef:
					_gcWalk(*entityRef.asLocalVar.context);
					_gcWalk(readVarUnsafe(entityRef));
					break;
				case ObjectRefKind::CoroutineLocalVarRef:
					GCWalkContext::pushObject(entityRef.asCoroutineLocalVar.coroutine);
					_gcWalk(readVarUnsafe(entityRef));
					break;
				case ObjectRefKind::ArgRef:
					break;
				case ObjectRefKind::CoroutineArgRef:
					GCWalkContext::pushObject(entityRef.asCoroutineArg.coroutine);
					GCWalkContext::pushObject(entityRef.asCoroutineArg.coroutine);
					break;
			}
			break;
		}
		case ValueType::RegRef:
			break;
		case ValueType::TypeName:
			_gcWalk(i.getTypeName());
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

SLAKE_API void GCWalkContext::removeFromUnwalkedList(Object *v) {
	GCWalkContext &context = *v->gcInfo.heapless.gcWalkContext;

	std::lock_guard accessMutexGuard(context.accessMutex);

	if (v == context.unwalkedList) {
		context.unwalkedList = v->gcInfo.heapless.nextUnwalked;
	}
	if (v->gcInfo.heapless.nextUnwalked) {
		v->gcInfo.heapless.nextUnwalked->gcInfo.heapless.prevUnwalked = v->gcInfo.heapless.prevUnwalked;
	}
	if (v->gcInfo.heapless.prevUnwalked) {
		v->gcInfo.heapless.prevUnwalked->gcInfo.heapless.nextUnwalked = v->gcInfo.heapless.nextUnwalked;
	}

	v->gcInfo.heapless.nextUnwalked = nullptr;
	v->gcInfo.heapless.prevUnwalked = nullptr;
}

SLAKE_API void GCWalkContext::removeFromDestructibleList(Object *v) {
	GCWalkContext &context = *v->gcInfo.heapless.gcWalkContext;

	std::lock_guard accessMutexGuard(context.accessMutex);

	if (v->gcInfo.heapless.nextDestructible) {
		if (v == context.destructibleList) {
			context.destructibleList = v->gcInfo.heapless.nextDestructible;
		}
		v->gcInfo.heapless.nextDestructible->gcInfo.heapless.prevDestructible = v->gcInfo.heapless.prevDestructible;
	}
	if (v->gcInfo.heapless.prevDestructible) {
		if (v == context.destructibleList) {
			context.destructibleList = v->gcInfo.heapless.prevDestructible;
		}
		v->gcInfo.heapless.prevDestructible->gcInfo.heapless.nextDestructible = v->gcInfo.heapless.nextDestructible;
	}
}

SLAKE_API void Runtime::_gcWalk(Object *v) {
	if (!v)
		return;

	switch (v->gcInfo.heapless.gcStatus) {
		case ObjectGCStatus::Unwalked:
			throw std::logic_error("Cannot walk on an unwalked object");
		case ObjectGCStatus::ReadyToWalk:
			v->gcInfo.heapless.gcStatus = ObjectGCStatus::Walked;

			switch (auto typeId = v->getKind(); typeId) {
				case ObjectKind::String:
					break;
				case ObjectKind::TypeDef:
					_gcWalk(((TypeDefObject *)v)->type);
					break;
				case ObjectKind::FnTypeDef: {
					auto typeDef = ((FnTypeDefObject *)v);
					_gcWalk(typeDef->returnType);
					for (auto &i : typeDef->paramTypes)
						_gcWalk(i);
					break;
				}
				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					GCWalkContext::pushObject(value->_class);
					if (auto mt = value->_class->cachedInstantiatedMethodTable) {
						_gcWalk(mt);
					}

					if (value->_class->cachedObjectLayout) {
						for (auto &i : value->_class->cachedObjectLayout->fieldRecords) {
							_gcWalk(i.type);

							switch (i.type.typeId) {
								case TypeId::String:
								case TypeId::Instance:
								case TypeId::Array:
								case TypeId::Ref:
									GCWalkContext::pushObject(*((Object **)(value->rawFieldData + i.offset)));
									break;
							}
						}
					}

					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gcWalk(value->elementType);

					switch (value->elementType.typeId) {
						case TypeId::Instance: {
							for (size_t i = 0; i < value->length; ++i)
								GCWalkContext::pushObject(((Object **)value->data)[i]);
							break;
						}
					}
					break;
				}
				case ObjectKind::Module: {
					for (auto i = ((ModuleObject *)v)->members.begin(); i != ((ModuleObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(i.value());
					}
					for (size_t i = 0; i < ((ModuleObject *)v)->fieldRecords.size(); ++i) {
						_gcWalk(readVarUnsafe(EntityRef::makeFieldRef((ModuleObject *)v, i)));
					}

					GCWalkContext::pushObject(((ModuleObject *)v)->parent);

					for (auto i : ((ModuleObject *)v)->unnamedImports)
						GCWalkContext::pushObject(i);

					break;
				}
				case ObjectKind::Class: {
					for (auto i = ((ClassObject *)v)->members.begin(); i != ((ClassObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(i.value());
					}
					GCWalkContext::pushObject(((ClassObject *)v)->parent);

					ClassObject *value = (ClassObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(readVarUnsafe(EntityRef::makeFieldRef(value, i)));
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						_gcWalk(i);
					}
					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalk(j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalk(i);
					}

					// value->parentClass.loadDeferredType(this);
					_gcWalk(value->baseType);

					_gcWalk(value->genericParams);

					break;
				}
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.
					for (auto i = ((InterfaceObject *)v)->members.begin(); i != ((InterfaceObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(i.value());
					}
					GCWalkContext::pushObject(((InterfaceObject *)v)->parent);

					InterfaceObject *value = (InterfaceObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(readVarUnsafe(EntityRef::makeFieldRef(value, i)));
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						GCWalkContext::pushObject(i.getCustomTypeExData());
					}
					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(i.baseType);
						for (auto &j : i.interfaces) {
							_gcWalk(j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalk(i);
					}

					_gcWalk(value->genericParams);

					break;
				}
				case ObjectKind::Fn: {
					auto fn = (FnObject *)v;

					GCWalkContext::pushObject(fn->parent);

					for (auto i : fn->overloadings) {
						GCWalkContext::pushObject(i);
					}

					break;
				}
				case ObjectKind::FnOverloading: {
					auto fnOverloading = (FnOverloadingObject *)v;

					GCWalkContext::pushObject(fnOverloading->fnObject);

					for (auto &i : fnOverloading->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalk(j);
						}
					}
					for (auto i = fnOverloading->mappedGenericArgs.begin(); i != fnOverloading->mappedGenericArgs.end(); ++i) {
						// i.value().loadDeferredType(this);
						_gcWalk(i.value());
					}

					for (auto &j : fnOverloading->paramTypes)
						_gcWalk(j);
					_gcWalk(fnOverloading->returnType);

					switch (fnOverloading->overloadingKind) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fnOverloading;

							for (auto &i : ol->instructions) {
								for (size_t j = 0; j < i.nOperands; ++j) {
									_gcWalk(i.operands[j]);
								}
							}

							break;
						}
						case FnOverloadingKind::Native: {
							NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)fnOverloading;

							break;
						}
						default:
							throw std::logic_error("Invalid overloading kind");
					}

					_gcWalk(fnOverloading->genericParams);

					break;
				}
				case ObjectKind::IdRef: {
					auto value = (IdRefObject *)v;

					for (auto &i : value->entries) {
						for (auto &j : i.genericArgs) {
							_gcWalk(j);
						}
					}

					if (value->paramTypes) {
						for (auto &j : *value->paramTypes) {
							_gcWalk(j);
						}
					}
					break;
				}
				case ObjectKind::Context: {
					auto value = (ContextObject *)v;

					_gcWalk(value->_context);
					break;
				}
				case ObjectKind::Coroutine: {
					auto value = (CoroutineObject *)v;

					GCWalkContext::pushObject((FnOverloadingObject *)value->overloading);
					GCWalkContext::pushObject(value->resumable.thisObject);
					for (auto &k : value->resumable.argStack) {
						_gcWalk(k.type);
						_gcWalk(k.value);
					}
					for (auto &k : value->resumable.nextArgStack)
						_gcWalk(k);
					for (auto &k : value->resumable.minorFrames) {
						for (auto &l : k.exceptHandlers) {
							_gcWalk(l.type);
						}
					}
					if (value->stackData) {
						for (size_t i = 0; i < value->resumable.nRegs; ++i)
							_gcWalk(*((Value *)(value->stackData + value->lenStackData - (value->offStackTop + sizeof(Value) * i))));
					}
					if (value->isDone()) {
						_gcWalk(value->finalResult);
					}
					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}

			GCWalkContext::removeFromUnwalkedList(v);
			GCWalkContext::removeFromDestructibleList(v);

			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcWalk(char *dataStack, MajorFrame *majorFrame) {
	GCWalkContext::pushObject((FnOverloadingObject *)majorFrame->curFn);
	if (majorFrame->curCoroutine)
		GCWalkContext::pushObject(majorFrame->curCoroutine);
	GCWalkContext::pushObject(majorFrame->resumable.thisObject);
	_gcWalk(majorFrame->curExcept);
	for (auto &k : majorFrame->resumable.argStack) {
		_gcWalk(k.type);
		_gcWalk(k.value);
	}
	for (auto &k : majorFrame->resumable.nextArgStack)
		_gcWalk(k);
	for (size_t i = 0; i < majorFrame->resumable.nRegs; ++i)
		_gcWalk(*((Value *)(dataStack + SLAKE_STACK_MAX - (majorFrame->offRegs + sizeof(Value) * i))));
	for (auto &k : majorFrame->resumable.minorFrames) {
		for (auto &l : k.exceptHandlers)
			_gcWalk(l.type);
	}
}

SLAKE_API void Runtime::_gcWalk(Context &ctxt) {
	bool isWalkableObjectDetected = false;
	for (auto &i : ctxt.majorFrames) {
		MajorFrame *majorFrame = i.get();
		_gcWalk(majorFrame);
	}
}

SLAKE_API void GCWalkContext::pushObject(Object *object) {
	if (!object)
		return;

	GCWalkContext &context = *object->gcInfo.heapless.gcWalkContext;

	switch (object->gcInfo.heapless.gcStatus) {
		case ObjectGCStatus::Unwalked:
			object->gcInfo.heapless.gcStatus = ObjectGCStatus::ReadyToWalk;

			context.pushWalkable(object);
			break;
		case ObjectGCStatus::ReadyToWalk:
			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API bool GCWalkContext::isWalkableListEmpty() {
	return !walkableList;
}

SLAKE_API Object *GCWalkContext::getWalkableList() {
	std::lock_guard accessMutexGuard(accessMutex);

	Object *p = walkableList;
	walkableList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushWalkable(Object *walkableObject) {
	std::lock_guard accessMutexGuard(accessMutex);

	walkableObject->gcInfo.heapless.nextWalkable = walkableList;
	walkableList = walkableObject;
}

SLAKE_API Object *GCWalkContext::getUnwalkedList() {
	std::lock_guard accessMutexGuard(accessMutex);

	Object *p = unwalkedList;
	unwalkedList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushUnwalked(Object *walkableObject) {
	std::lock_guard accessMutexGuard(accessMutex);

	if (unwalkedList)
		unwalkedList->gcInfo.heapless.prevUnwalked = walkableObject;

	walkableObject->gcInfo.heapless.nextUnwalked = unwalkedList;

	unwalkedList = walkableObject;
}

SLAKE_API InstanceObject *GCWalkContext::getDestructibleList() {
	std::lock_guard accessMutexGuard(accessMutex);

	auto p = destructibleList;
	destructibleList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushDestructible(InstanceObject *v) {
	std::lock_guard accessMutexGuard(accessMutex);

	if (destructibleList)
		destructibleList->gcInfo.heapless.prevDestructible = v;

	v->gcInfo.heapless.nextDestructible = destructibleList;

	destructibleList = v;
}

SLAKE_API void GCWalkContext::reset() {
	walkableList = nullptr;
	unwalkedInstanceList = nullptr;
	destructibleList = nullptr;
	unwalkedList = nullptr;
}

SLAKE_API void Runtime::_gc() {
rescan:
	GCWalkContext context;

	Object *hostRefList = nullptr;

	{
		for (auto i : createdObjects) {
			i->gcInfo.heapless.gcStatus = ObjectGCStatus::Unwalked;
			i->gcInfo.heapless.gcWalkContext = &context;

			i->gcInfo.heapless.nextWalkable = nullptr;
			i->gcInfo.heapless.prevDestructible = nullptr;
			i->gcInfo.heapless.nextDestructible = nullptr;

			i->gcInfo.heapless.nextHostRef = nullptr;

			i->gcInfo.heapless.nextUnwalked = nullptr;
			i->gcInfo.heapless.prevUnwalked = nullptr;

			// Check if the object is referenced by the host, if so, exclude them into a separated list.
			if (i->hostRefCount) {
				i->gcInfo.heapless.nextHostRef = hostRefList;
				hostRefList = i;
			}

			switch (i->getKind()) {
				case ObjectKind::Instance: {
					InstanceObject *value = (InstanceObject *)i;

					if (!(value->_flags & VF_DESTRUCTED)) {
						context.pushDestructible(value);
					}
					break;
				}
				default:
					break;
			}

			context.pushUnwalked(i);
		}
	}

	for (Object *i = hostRefList, *next; i; i = next) {
		next = i->gcInfo.heapless.nextHostRef;
		GCWalkContext::pushObject(i);
		i->gcInfo.heapless.nextHostRef = nullptr;
	}

	for (auto i = _genericCacheDir.begin(); i != _genericCacheDir.end(); ++i) {
		for (auto j = i.value().begin(); j != i.value().end(); ++j) {
			GCWalkContext::pushObject(j.value());
		}
	}

	for (auto i : managedThreadRunnables) {
		GCWalkContext::pushObject(i.second->context.get());
	}

	if (!(_flags & _RT_DEINITING)) {
		// Walk the root node.
		GCWalkContext::pushObject(_rootObject);

		// Walk contexts for each thread.
		for (auto &i : activeContexts)
			GCWalkContext::pushObject(i.second);
	}

	for (Object *p = context.getWalkableList(), *i; p;) {
		i = p;

		while (i) {
			Object *next = i->gcInfo.heapless.nextWalkable;
			switch (i->gcInfo.heapless.gcStatus) {
				case ObjectGCStatus::Unwalked:
					assert(false);
					break;
				case ObjectGCStatus::ReadyToWalk:
					_gcWalk(i);
					break;
				case ObjectGCStatus::Walked:
					assert(false);
					break;
			}
			i = next;
		}

		p = context.getWalkableList();
	}

	if (InstanceObject *p = context.getDestructibleList(); p) {
		_destructDestructibleObjects(p);
		goto rescan;
	}

	// Delete unreachable objects.
	for (Object *i = context.getUnwalkedList(), *next; i; i = next) {
		next = i->gcInfo.heapless.nextUnwalked;

		i->dealloc();
		createdObjects.remove(i);
	}
}

SLAKE_API void Runtime::ParallelGcThreadRunnable::run() {
	for (;;) {
		while (!isActive)
			yieldCurrentThread();

		isActive = false;

		if (threadState == ParallelGcThreadState::NotifyTermination) {
			break;
		}

		for (Object *p = context.getWalkableList(), *i; p;) {
			i = p;

			while (i) {
				Object *next = i->gcInfo.heapless.nextWalkable;
				switch (i->gcInfo.heapless.gcStatus) {
					case ObjectGCStatus::Unwalked:
						assert(false);
						break;
					case ObjectGCStatus::ReadyToWalk:
						runtime->_gcWalk(i);
						break;
					case ObjectGCStatus::Walked:
						assert(false);
						break;
				}
				i = next;
			}

			p = context.getWalkableList();
		}

		isDone = true;
		while (isDone) {
			yieldCurrentThread();
		}
	}

	threadState = ParallelGcThreadState::Terminated;
	return;
}

SLAKE_API void Runtime::ParallelGcThreadRunnable::dealloc() {
	peff::destroyAndRelease<ParallelGcThreadRunnable>(&runtime->globalHeapPoolAlloc, this, alignof(ParallelGcThreadRunnable));
}

SLAKE_API void Runtime::_gcParallelHeapless() {
rescan:
	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		curRunnable->context.reset();
	}

	Object *hostRefList = nullptr;

	{
		auto curObjIt = createdObjects.begin();

		size_t stepRemainder = createdObjects.size() % nMaxGcThreads;
		size_t step = createdObjects.size() / nMaxGcThreads + (stepRemainder > 0);
		for (size_t idx = 0, j = 0; j < createdObjects.size(); j += step, ++idx) {
			GCWalkContext &context = parallelGcThreadRunnables.at(idx)->context;

			{
				size_t nObjects = createdObjects.size() - j;

				if (nObjects > step) {
					nObjects = step;
				}

				for (size_t i = 0; i < nObjects; ++i) {
					const auto cur = curObjIt++;

					(*cur)->gcInfo.heapless.gcStatus = ObjectGCStatus::Unwalked;
					(*cur)->gcInfo.heapless.gcWalkContext = &context;

					(*cur)->gcInfo.heapless.nextWalkable = nullptr;
					(*cur)->gcInfo.heapless.prevDestructible = nullptr;
					(*cur)->gcInfo.heapless.nextDestructible = nullptr;

					(*cur)->gcInfo.heapless.nextHostRef = nullptr;

					(*cur)->gcInfo.heapless.nextUnwalked = nullptr;
					(*cur)->gcInfo.heapless.prevUnwalked = nullptr;

					// Check if the object is referenced by the host, if so, exclude them into a separated list.
					if ((*cur)->hostRefCount) {
						(*cur)->gcInfo.heapless.nextHostRef = hostRefList;
						hostRefList = (*cur);
					}

					switch ((*cur)->getKind()) {
						case ObjectKind::Instance: {
							InstanceObject *value = (InstanceObject *)(*cur);

							if (!(value->_flags & VF_DESTRUCTED)) {
								context.pushDestructible(value);
							}
							break;
						}
						default:
							break;
					}

					context.pushUnwalked((*cur));
				}
			}
		}
	}
	
	for (Object *i = hostRefList, *next; i; i = next) {
		next = i->gcInfo.heapless.nextHostRef;
		GCWalkContext::pushObject(i);
		i->gcInfo.heapless.nextHostRef = nullptr;
	}

	for (auto i = _genericCacheDir.begin(); i != _genericCacheDir.end(); ++i) {
		for (auto j = i.value().begin(); j != i.value().end(); ++j) {
			GCWalkContext::pushObject(j.value());
		}
	}

	for (auto i : managedThreadRunnables) {
		GCWalkContext::pushObject(i.second->context.get());
	}

	if (!(_flags & _RT_DEINITING)) {
		// Walk the root node.
		GCWalkContext::pushObject(_rootObject);

		// Walk contexts for each thread.
		for (auto &i : activeContexts)
			GCWalkContext::pushObject(i.second);
	}

rescanLeftovers:
	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		curRunnable->isActive = true;
	}

	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		while (!curRunnable->isDone) {
			yieldCurrentThread();
		}
		curRunnable->isDone = false;
	}

	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		if (!curRunnable->context.isWalkableListEmpty()) {
			goto rescanLeftovers;
		}
	}

	bool isRescanNeeded = false;

	for (size_t i = 0; i < parallelGcThreads.size(); ++i) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)parallelGcThreads.at(i)->runnable;
		GCWalkContext &context = curRunnable->context;

		if (InstanceObject *p = context.getDestructibleList(); p) {
			_destructDestructibleObjects(p);
			isRescanNeeded = true;
		}
	}

	for (size_t i = 0; i < parallelGcThreads.size(); ++i) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)parallelGcThreads.at(i)->runnable;
		GCWalkContext &context = curRunnable->context;

		// Delete unreachable objects.
		for (Object *j = context.getUnwalkedList(), *next; j; j = next) {
			next = j->gcInfo.heapless.nextUnwalked;

			j->dealloc();
			createdObjects.remove(j);
		}
	}
	
	if (isRescanNeeded) {
		goto rescan;
	}
}

SLAKE_API Runtime::ParallelGcThreadRunnable::ParallelGcThreadRunnable(Runtime *runtime) : runtime(runtime) {
}

SLAKE_API bool Runtime::_allocParallelGcResources() {
	if (!parallelGcThreadRunnables.resize(nMaxGcThreads)) {
		return false;
	}

	for (size_t i = 0; i < nMaxGcThreads; ++i) {
		auto &p = parallelGcThreadRunnables.at(i);

		if (!(p = std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>>(peff::allocAndConstruct<ParallelGcThreadRunnable>(&globalHeapPoolAlloc, alignof(ParallelGcThreadRunnable), this)))) {
			return false;
		}
	}

	if (!parallelGcThreads.resize(nMaxGcThreads)) {
		return false;
	}

	for (size_t i = 0; i < nMaxGcThreads; ++i) {
		if (!(parallelGcThreads.at(i) = std::unique_ptr<Thread, util::DeallocableDeleter<Thread>>(Thread::alloc(&globalHeapPoolAlloc, parallelGcThreadRunnables.at(i).get(), 4096)))) {
			return false;
		}
	}

	for (size_t i = 0; i < nMaxGcThreads; ++i) {
		parallelGcThreads.at(i)->start();
	}

	return true;
}

SLAKE_API void Runtime::_releaseParallelGcResources() {
	for (auto &i : parallelGcThreadRunnables) {
		i->threadState = ParallelGcThreadState::NotifyTermination;

		i->isActive = true;

		while (i->threadState == ParallelGcThreadState::NotifyTermination)
			yieldCurrentThread();
	}

	parallelGcThreads.clear();
	parallelGcThreadRunnables.clear();
}
