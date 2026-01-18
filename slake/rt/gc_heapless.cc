#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto i : methodTable->methods) {
		context->pushObject(i.second);
	}
	for (auto i : methodTable->destructors) {
		context->pushObject(i);
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, GenericParamList &genericParamList) {
	for (auto &i : genericParamList) {
		// i.baseType.loadDeferredType(this);
		_gcWalk(context, i.baseType);

		for (auto &j : i.interfaces) {
			// j.loadDeferredType(this);
			_gcWalk(context, i.baseType);
		}
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, const TypeRef &type) {
	context->pushObject(type.typeDef);
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, const Value &i) {
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
		case ValueType::Reference: {
			const Reference &entityRef = i.getReference();

			switch (entityRef.kind) {
				case ReferenceKind::StaticFieldRef:
					context->pushObject(entityRef.asStaticField.moduleObject);
					break;
				case ReferenceKind::ArrayElementRef:
					context->pushObject(entityRef.asArrayElement.arrayObject);
					break;
				case ReferenceKind::ObjectRef:
					context->pushObject(entityRef.asObject);
					break;
				case ReferenceKind::InstanceFieldRef:
					context->pushObject(entityRef.asObjectField.instanceObject);
					break;
				case ReferenceKind::LocalVarRef: {
					Value data;
					_gcWalk(context, *entityRef.asLocalVar.context);
					readVar(entityRef, data);
					_gcWalk(context, data);
					break;
				}
				case ReferenceKind::CoroutineLocalVarRef: {
					Value data;
					context->pushObject(entityRef.asCoroutineLocalVar.coroutine);
					readVar(entityRef, data);
					_gcWalk(context, data);
					break;
				}
				case ReferenceKind::ArgRef:
					break;
				case ReferenceKind::CoroutineArgRef:
					context->pushObject(entityRef.asCoroutineArg.coroutine);
					break;
			}
			break;
		}
		case ValueType::RegIndex:
			break;
		case ValueType::TypeName:
			_gcWalk(context, i.getTypeName());
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

SLAKE_API void GCWalkContext::removeFromWalkableList(Object *v) {
	MutexGuard accessMutexGuard(accessMutex);

	if (v == walkableList)
		walkableList = v->nextSameGCSet;

	removeFromCurGCSet(v);
}

SLAKE_API void GCWalkContext::removeFromDestructibleList(Object *v) {
	MutexGuard accessMutexGuard(accessMutex);
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, Object *v) {
	if (!v)
		return;

	switch (v->gcStatus) {
		case ObjectGCStatus::Unwalked:
			throw std::logic_error("Cannot walk on an unwalked object");
		case ObjectGCStatus::ReadyToWalk:
			v->gcStatus = ObjectGCStatus::Walked;

			switch (v->getObjectKind()) {
				case ObjectKind::String:
					break;
				case ObjectKind::HeapType:
					_gcWalk(context, ((HeapTypeObject *)v)->typeRef);
					break;
				case ObjectKind::TypeDef: {
					auto typeDef = (TypeDefObject *)v;

					switch (typeDef->getTypeDefKind()) {
						case TypeDefKind::CustomTypeDef:
							context->pushObject(((CustomTypeDefObject *)typeDef)->typeObject);
							break;
						case TypeDefKind::ArrayTypeDef:
							context->pushObject(((ArrayTypeDefObject *)typeDef)->elementType);
							break;
						case TypeDefKind::RefTypeDef:
							context->pushObject(((RefTypeDefObject *)typeDef)->referencedType);
							break;
						case TypeDefKind::GenericArgTypeDef:
							context->pushObject(((GenericArgTypeDefObject *)typeDef)->ownerObject);
							context->pushObject(((GenericArgTypeDefObject *)typeDef)->nameObject);
							break;
						case TypeDefKind::FnTypeDef: {
							auto td = ((FnTypeDefObject *)typeDef);
							context->pushObject(td->returnType);
							for (auto i : td->paramTypes)
								context->pushObject(i);
							break;
						}
						case TypeDefKind::ParamTypeListTypeDef: {
							auto td = ((ParamTypeListTypeDefObject *)typeDef);
							for (auto i : td->paramTypes)
								context->pushObject(i);
							break;
						}
						case TypeDefKind::TupleTypeDef: {
							auto td = ((TupleTypeDefObject *)typeDef);
							for (auto i : td->elementTypes)
								context->pushObject(i);
							break;
						}
						case TypeDefKind::SIMDTypeDef: {
							auto td = ((SIMDTypeDefObject *)typeDef);
							context->pushObject(td->type);
							break;
						}
						case TypeDefKind::UnpackingTypeDef: {
							auto td = ((UnpackingTypeDefObject *)typeDef);
							context->pushObject(td->type);
							break;
						}
						default:
							std::terminate();
					}
					break;
				}
				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					context->pushObject(value->_class);

					if (value->_class->cachedObjectLayout) {
						for (auto &i : value->_class->cachedObjectLayout->fieldRecords) {
							switch (i.type.typeId) {
								case TypeId::String:
								case TypeId::Instance:
								case TypeId::Array:
								case TypeId::Ref:
									context->pushObject(*((Object **)(value->rawFieldData + i.offset)));
									break;
							}
						}
					}

					context->removeFromDestructibleList(value);
					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gcWalk(context, value->elementType);

					switch (value->elementType.typeId) {
						case TypeId::Instance: {
							for (size_t i = 0; i < value->length; ++i)
								context->pushObject(((Object **)value->data)[i]);
							break;
						}
					}
					break;
				}
				case ObjectKind::Module: {
					for (auto i = ((ModuleObject *)v)->members.begin(); i != ((ModuleObject *)v)->members.end(); ++i) {
						context->pushObject(i.value());
					}
					Value data;
					for (size_t i = 0; i < ((ModuleObject *)v)->fieldRecords.size(); ++i) {
						readVar(Reference::makeStaticFieldRef((ModuleObject *)v, i), data);
						_gcWalk(context, data);
					}

					context->pushObject(((ModuleObject *)v)->getParent());

					for (auto i : ((ModuleObject *)v)->unnamedImports)
						context->pushObject(i);

					break;
				}
				case ObjectKind::Class: {
					for (auto i = ((ClassObject *)v)->members.begin(); i != ((ClassObject *)v)->members.end(); ++i) {
						context->pushObject(i.value());
					}
					context->pushObject(((ClassObject *)v)->getParent());

					ClassObject *value = (ClassObject *)v;

					if (auto mt = value->cachedInstantiatedMethodTable; mt) {
						_gcWalk(context, mt);
					}

					Value data;
					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, value->fieldRecords.at(i).type);
						readVar(Reference::makeStaticFieldRef(value, i), data);
						_gcWalk(context, data);
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						_gcWalk(context, i);
					}
					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(context, i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalk(context, j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalk(context, i);
					}

					// value->parentClass.loadDeferredType(this);
					_gcWalk(context, value->baseType);

					_gcWalk(context, value->genericParams);

					break;
				}
				case ObjectKind::Struct: {
					for (auto i = ((StructObject *)v)->members.begin(); i != ((StructObject *)v)->members.end(); ++i) {
						context->pushObject(i.value());
					}
					context->pushObject(((StructObject *)v)->getParent());

					StructObject *value = (StructObject *)v;

					Value data;
					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, value->fieldRecords.at(i).type);
						readVar(Reference::makeStaticFieldRef(value, i), data);
						_gcWalk(context, data);
					}

					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(context, i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalk(context, j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalk(context, i);
					}

					_gcWalk(context, value->genericParams);

					break;
				}
				case ObjectKind::ScopedEnum: {
					for (auto i = ((ScopedEnumObject *)v)->members.begin(); i != ((ScopedEnumObject *)v)->members.end(); ++i) {
						context->pushObject(i.value());
					}
					context->pushObject(((ScopedEnumObject *)v)->getParent());

					ScopedEnumObject *value = (ScopedEnumObject *)v;

					if (value->baseType) {
						Value data;
						for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
							_gcWalk(context, value->fieldRecords.at(i).type);
							readVar(Reference::makeStaticFieldRef(value, i), data);
							_gcWalk(context, data);
						}
					}

					break;
				}
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.
					for (auto i = ((InterfaceObject *)v)->members.begin(); i != ((InterfaceObject *)v)->members.end(); ++i) {
						context->pushObject(i.value());
					}
					context->pushObject(((InterfaceObject *)v)->getParent());

					InterfaceObject *value = (InterfaceObject *)v;

					Value data;
					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						readVar(Reference::makeStaticFieldRef(value, i), data);
						_gcWalk(context, data);
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						context->pushObject(i.typeDef);
					}
					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(context, i.baseType);
						for (auto &j : i.interfaces) {
							_gcWalk(context, j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalk(context, i);
					}

					_gcWalk(context, value->genericParams);

					break;
				}
				case ObjectKind::Fn: {
					auto fn = (FnObject *)v;

					context->pushObject(fn->getParent());

					for (auto i : fn->overloadings) {
						context->pushObject(i.second);
					}

					break;
				}
				case ObjectKind::FnOverloading: {
					auto fnOverloading = (FnOverloadingObject *)v;

					context->pushObject(fnOverloading->fnObject);

					for (auto &i : fnOverloading->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalk(context, i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalk(context, j);
						}
					}
					for (auto i = fnOverloading->mappedGenericArgs.begin(); i != fnOverloading->mappedGenericArgs.end(); ++i) {
						// i.value().loadDeferredType(this);
						_gcWalk(context, i.value());
					}

					for (auto &j : fnOverloading->paramTypes)
						_gcWalk(context, j);
					_gcWalk(context, fnOverloading->returnType);

					switch (fnOverloading->overloadingKind) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fnOverloading;

							for (auto &i : ol->instructions) {
								for (size_t j = 0; j < i.nOperands; ++j) {
									_gcWalk(context, i.operands[j]);
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

					_gcWalk(context, fnOverloading->genericParams);

					break;
				}
				case ObjectKind::IdRef: {
					auto value = (IdRefObject *)v;

					for (auto &i : value->entries) {
						for (auto &j : i.genericArgs) {
							_gcWalk(context, j);
						}
					}

					if (value->paramTypes.hasValue()) {
						for (auto &j : *value->paramTypes) {
							_gcWalk(context, j);
						}
					}
					break;
				}
				case ObjectKind::Context: {
					auto value = (ContextObject *)v;

					_gcWalk(context, value->_context);
					break;
				}
				case ObjectKind::Coroutine: {
					auto value = (CoroutineObject *)v;

					context->pushObject((FnOverloadingObject *)value->overloading);

					if (value->resumable) {
						context->pushObject(value->resumable);

						if (value->stackData) {
							for (size_t i = 0; i < value->resumable->nRegs; ++i)
								_gcWalk(context, *((Value *)(value->stackData + value->lenStackData - sizeof(Value) * i)));
						}
					}

					if (value->isDone()) {
						_gcWalk(context, value->finalResult);
					}
					break;
				}
				case ObjectKind::Resumable: {
					auto value = (ResumableObject *)v;

					context->pushObject(value->thisObject);
					for (auto &k : value->argStack) {
						_gcWalk(context, k.type);
						_gcWalk(context, k.value);
					}
					for (auto &k : value->nextArgStack)
						_gcWalk(context, k);
					for (auto &k : value->minorFrames) {
						for (auto &l : k.exceptHandlers)
							_gcWalk(context, l.type);
					}

					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}

			context->removeFromWalkableList(v);
			context->pushWalked(v);

			v->gcSpinlock.unlock();

			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, char *dataStack, size_t stackSize, MajorFrame *majorFrame) {
	context->pushObject((FnOverloadingObject *)majorFrame->curFn);
	if (majorFrame->curCoroutine)
		context->pushObject(majorFrame->curCoroutine);

	context->pushObject(majorFrame->resumable);

	if (majorFrame->resumable) {
		size_t nRegs = majorFrame->resumable->nRegs;
		for (size_t i = 0; i < nRegs; ++i)
			_gcWalk(context, calcStackAddr(dataStack, stackSize, (majorFrame->offRegs + sizeof(Value) * i)));
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, Context &ctxt) {
	bool isWalkableObjectDetected = false;

	for (auto &i : ctxt.majorFrames) {
		_gcWalk(context, ctxt.dataStack, ctxt.stackSize, i.get());
	}
}

SLAKE_API void GCWalkContext::removeFromCurGCSet(Object *object) {
	if (object->prevSameGCSet)
		object->prevSameGCSet->nextSameGCSet = object->nextSameGCSet;
	if (object->nextSameGCSet)
		object->nextSameGCSet->prevSameGCSet = object->prevSameGCSet;
	object->nextSameGCSet = nullptr;
	object->prevSameGCSet = nullptr;
}

SLAKE_API void GCWalkContext::pushObject(Object *object) {
	if (!object)
		return;

	switch (object->gcStatus) {
		case ObjectGCStatus::Unwalked:
			object->gcStatus = ObjectGCStatus::ReadyToWalk;

			object->gcSpinlock.lock();

			if (object == unwalkedList)
				unwalkedList = object->nextSameGCSet;

			removeFromCurGCSet(object);
			pushWalkable(object);
			break;
		case ObjectGCStatus::ReadyToWalk:
			break;
		case ObjectGCStatus::Walked:
			break;
		default:
			std::terminate();
	}
}

SLAKE_API bool GCWalkContext::isWalkableListEmpty() {
	return !walkableList;
}

SLAKE_API Object *GCWalkContext::getWalkableList() {
	MutexGuard accessMutexGuard(accessMutex);

	Object *p = walkableList;
	walkableList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushWalkable(Object *walkableObject) {
	MutexGuard accessMutexGuard(accessMutex);

	if (unwalkedList == walkableObject)
		unwalkedList = walkableObject->nextSameGCSet;

	walkableObject->nextSameGCSet = walkableList;
	walkableList = walkableObject;
}

SLAKE_API Object *GCWalkContext::getUnwalkedList(bool clearList) {
	MutexGuard accessMutexGuard(accessMutex);

	Object *p = unwalkedList;
	if (clearList)
		unwalkedList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushUnwalked(Object *walkableObject) {
	MutexGuard accessMutexGuard(accessMutex);

	if (unwalkedList)
		unwalkedList->prevSameGCSet = walkableObject;

	walkableObject->nextSameGCSet = unwalkedList;

	unwalkedList = walkableObject;
}

SLAKE_API void GCWalkContext::updateUnwalkedList(Object *deletedObject) {
	MutexGuard accessMutexGuard(accessMutex);

	if (unwalkedList == deletedObject)
		unwalkedList = deletedObject->nextSameGCSet;

	removeFromCurGCSet(deletedObject);
}

SLAKE_API Object *GCWalkContext::getWalkedList() {
	return walkedList;
}

SLAKE_API void GCWalkContext::pushWalked(Object *walkedObject) {
	MutexGuard accessMutexGuard(accessMutex);

	if (walkedList)
		walkedList->prevSameGCSet = walkedObject;

	walkedObject->nextSameGCSet = walkedList;

	walkedList = walkedObject;
}

SLAKE_API InstanceObject *GCWalkContext::getDestructibleList() {
	MutexGuard accessMutexGuard(accessMutex);

	auto p = destructibleList;
	destructibleList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushDestructible(InstanceObject *v) {
	MutexGuard accessMutexGuard(accessMutex);
}

SLAKE_API void GCWalkContext::reset() {
	walkableList = nullptr;
	unwalkedInstanceList = nullptr;
	destructibleList = nullptr;
	unwalkedList = nullptr;
}

enum class GCTarget : uint8_t {
	TypeDef = 0,
	All
};

SLAKE_API void Runtime::_gcSerial(Object *&objectList, Object *&endObjectOut, size_t &nObjects, ObjectGeneration newGeneration) {
	size_t iterationTimes = 0;
rescan:
	++iterationTimes;
	GCWalkContext context;

	Object *hostRefList = nullptr;

	{
		Object *prev = nullptr;

		for (Object *i = objectList; i; i = i->nextSameGenObject) {
			i->gcStatus = ObjectGCStatus::Unwalked;

			i->nextSameGCSet = nullptr;
			i->prevSameGCSet = nullptr;

			i->objectGeneration = newGeneration;

			// Check if the object is referenced by the host, if so, exclude them into a separated list.
			if (i->hostRefCount) {
				if (hostRefList)
					hostRefList->prevSameGCSet = i;
				i->nextSameGCSet = hostRefList;
				hostRefList = i;
			} else {
				context.pushUnwalked(i);
			}

			switch (i->getObjectKind()) {
				case ObjectKind::Instance: {
					InstanceObject *value = (InstanceObject *)i;

					/* if (!(value->_flags & VF_DESTRUCTED)) {
						context.pushDestructible(value);
					}*/
					break;
				}
				default:
					break;
			}

			prev = i;
		}

		endObjectOut = prev;
	}

	for (Object *i = hostRefList, *next; i; i = next) {
		next = i->nextSameGCSet;
		context.pushObject(i);
	}

	for (auto i : _genericCacheDir) {
		for (auto j : i.second) {
			context.pushObject(i.first);
			context.pushObject(j.second);
		}
	}

	for (auto i : managedThreadRunnables) {
		context.pushObject(i.second->context.get());
	}

	if (!(runtimeFlags & _RT_DEINITING)) {
		// Walk the root node.
		context.pushObject(_rootObject);

		// Walk contexts for each thread.
		for (auto &i : activeContexts)
			context.pushObject(i.second);
	}

	for (Object *p = context.getWalkableList(), *i; p;) {
		i = p;

		while (i) {
			Object *next = i->nextSameGCSet;

			switch (i->gcStatus) {
				case ObjectGCStatus::Unwalked:
					std::terminate();
					break;
				case ObjectGCStatus::ReadyToWalk:
					_gcWalk(&context, i);
					break;
				case ObjectGCStatus::Walked:
					std::terminate();
					break;
			}

			i = next;
		}

		p = context.getWalkableList();
	}

	bool isRescanNeeded = false;

	/*
	if (InstanceObject *p = context.getDestructibleList(); p) {
		_destructDestructibleObjects(p);
		isRescanNeeded = true;
	}*/

	if (isRescanNeeded) {
		goto rescan;
	}

	size_t nDeletedObjects = 0;
	// Delete unreachable objects.
	GCTarget curGcTarget = GCTarget::TypeDef;
	bool clearUnwalkedList = false;

rescanDeletables:
	Object *i = context.getUnwalkedList(clearUnwalkedList);

	bool updateUnwalkedList;
	switch (curGcTarget) {
		case GCTarget::TypeDef: {
			if (!isTypeDefObject(i))
				updateUnwalkedList = true;
			else
				updateUnwalkedList = false;
			break;
		}
		case GCTarget::All:
			updateUnwalkedList = false;
			break;
		default:
			std::terminate();
	}

	for (Object *next; i; i = next) {
		next = i->nextSameGCSet;

		switch (curGcTarget) {
			case GCTarget::TypeDef:
				if (!isTypeDefObject(i))
					continue;
				unregisterTypeDef((TypeDefObject *)i);
				break;
			case GCTarget::All:
				break;
			default:
				std::terminate();
		}

		if (i == objectList) {
			objectList = i->nextSameGenObject;
			assert(!i->prevSameGenObject);
		}

		if (endObjectOut == i) {
			if (i->prevSameGenObject) {
				endObjectOut = i->prevSameGenObject;
			} else {
				endObjectOut = nullptr;
			}
			assert(!i->nextSameGenObject);
		}

		if (i->prevSameGenObject) {
			i->prevSameGenObject->nextSameGenObject = i->nextSameGenObject;
		}

		if (i->nextSameGenObject) {
			i->nextSameGenObject->prevSameGenObject = i->prevSameGenObject;
		}

		if (updateUnwalkedList)
			context.updateUnwalkedList(i);
		i->dealloc();

		++nDeletedObjects;
	}

	switch (curGcTarget) {
		case GCTarget::TypeDef:
			curGcTarget = GCTarget::All;
			clearUnwalkedList = true;
			goto rescanDeletables;
		case GCTarget::All:
			break;
		default:
			std::terminate();
	}

	nObjects -= nDeletedObjects;

	for (Object *i = context.getWalkedList(), *next; i; i = next) {
		next = i->nextSameGCSet;
		context.removeFromCurGCSet(i);
		i->gcStatus = ObjectGCStatus::Unwalked;
	}
}

SLAKE_API void Runtime::ParallelGcThreadRunnable::run() {
	for (;;) {
		while (!isActive) {
			activeCond.wait();

			yieldCurrentThread();
		}

		isActive = false;

		if (threadState == ParallelGcThreadState::NotifyTermination) {
			break;
		}

		// TODO: Implement it.

		isDone = true;
		while (isDone) {
			doneCond.notifyAll();
			yieldCurrentThread();
		}
	}

	threadState = ParallelGcThreadState::Terminated;
	return;
}

SLAKE_API void Runtime::ParallelGcThreadRunnable::dealloc() {
	peff::destroyAndRelease<ParallelGcThreadRunnable>(runtime->getFixedAlloc(), this, alignof(ParallelGcThreadRunnable));
}

SLAKE_API void Runtime::_gcParallelHeapless(Object *&objectList, Object *&endObjectOut, size_t &nObjects, ObjectGeneration newGeneration) {
rescan:
	size_t nRecordedObjects = nObjects;

	parallelGcThreadRunnable->context.reset();
	parallelGcThreadRunnable->isActive = true;
	parallelGcThreadRunnable->activeCond.notifyAll();

	while (!parallelGcThreadRunnable->isDone) {
		yieldCurrentThread();
		parallelGcThreadRunnable->doneCond.wait();
	}
	parallelGcThreadRunnable->isDone = false;
}

SLAKE_API Runtime::ParallelGcThreadRunnable::ParallelGcThreadRunnable(Runtime *runtime) : runtime(runtime) {
}

SLAKE_API bool Runtime::_allocParallelGcResources() {
	if (!(parallelGcThreadRunnable = std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>>(peff::allocAndConstruct<ParallelGcThreadRunnable>(getFixedAlloc(), alignof(ParallelGcThreadRunnable), this)))) {
		return false;
	}

	if (!(parallelGcThread = std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>>(Thread::alloc(getFixedAlloc(), parallelGcThreadRunnable.get(), 4096)))) {
		return false;
	}

	parallelGcThread->start();

	return true;
}

SLAKE_API void Runtime::_releaseParallelGcResources() {
	parallelGcThreadRunnable->threadState = ParallelGcThreadState::NotifyTermination;

	parallelGcThreadRunnable->isActive = true;

	parallelGcThreadRunnable->activeCond.notifyAll();

	while (parallelGcThreadRunnable->threadState == ParallelGcThreadState::NotifyTermination)
		yieldCurrentThread();

	parallelGcThread.reset();
}
