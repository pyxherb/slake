#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto i : methodTable->methods) {
		GCWalkContext::pushObject(context, i.second);
	}
	for (auto i : methodTable->destructors) {
		GCWalkContext::pushObject(context, i);
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
	GCWalkContext::pushObject(context, type.typeDef);
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
		case ValueType::EntityRef: {
			const EntityRef &entityRef = i.getEntityRef();

			switch (entityRef.kind) {
				case EntityRefKind::StaticFieldRef:
					GCWalkContext::pushObject(context, entityRef.asStaticField.moduleObject);
					break;
				case EntityRefKind::ArrayElementRef:
					GCWalkContext::pushObject(context, entityRef.asArrayElement.arrayObject);
					break;
				case EntityRefKind::ObjectRef:
					GCWalkContext::pushObject(context, entityRef.asObject);
					break;
				case EntityRefKind::InstanceFieldRef:
					GCWalkContext::pushObject(context, entityRef.asObjectField.instanceObject);
					break;
				case EntityRefKind::LocalVarRef:
					_gcWalk(context, *entityRef.asLocalVar.context);
					_gcWalk(context, readVarUnsafe(entityRef));
					break;
				case EntityRefKind::CoroutineLocalVarRef:
					GCWalkContext::pushObject(context, entityRef.asCoroutineLocalVar.coroutine);
					_gcWalk(context, readVarUnsafe(entityRef));
					break;
				case EntityRefKind::ArgRef:
					break;
				case EntityRefKind::CoroutineArgRef:
					GCWalkContext::pushObject(context, entityRef.asCoroutineArg.coroutine);
					break;
			}
			break;
		}
		case ValueType::RegRef:
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

SLAKE_API void GCWalkContext::removeFromUnwalkedList(Object *v) {
	GCWalkContext &context = *v->gcWalkContext;

	MutexGuard accessMutexGuard(context.accessMutex);

	if (v == context.unwalkedList) {
		context.unwalkedList = v->nextUnwalked;
		assert(!v->prevUnwalked);
	}

	if (v->nextUnwalked) {
		v->nextUnwalked->prevUnwalked = v->prevUnwalked;
	}

	if (v->prevUnwalked) {
		v->prevUnwalked->nextUnwalked = v->nextUnwalked;
	}

	v->nextUnwalked = nullptr;
	v->prevUnwalked = nullptr;

	v->nextWalked = context.walkedList;
	context.walkedList = v;
}

SLAKE_API void GCWalkContext::removeFromDestructibleList(Object *v) {
	GCWalkContext &context = *v->gcWalkContext;

	MutexGuard accessMutexGuard(context.accessMutex);

	if (v->sameKindObjectList) {
		Runtime::removeSameKindObjectToList((Object **)&context.destructibleList, v);
		v->sameKindObjectList = nullptr;
	}
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
							GCWalkContext::pushObject(context, ((CustomTypeDefObject *)typeDef)->typeObject);
							break;
						case TypeDefKind::ArrayTypeDef:
							GCWalkContext::pushObject(context, ((ArrayTypeDefObject *)typeDef)->elementType);
							break;
						case TypeDefKind::RefTypeDef:
							GCWalkContext::pushObject(context, ((RefTypeDefObject *)typeDef)->referencedType);
							break;
						case TypeDefKind::GenericArgTypeDef:
							GCWalkContext::pushObject(context, ((GenericArgTypeDefObject *)typeDef)->ownerObject);
							GCWalkContext::pushObject(context, ((GenericArgTypeDefObject *)typeDef)->nameObject);
							break;
						case TypeDefKind::FnTypeDef: {
							auto td = ((FnTypeDefObject *)typeDef);
							GCWalkContext::pushObject(context, td->returnType);
							for (auto i : td->paramTypes)
								GCWalkContext::pushObject(context, i);
							break;
						}
						case TypeDefKind::ParamTypeListTypeDef: {
							auto td = ((ParamTypeListTypeDefObject *)typeDef);
							for (auto i : td->paramTypes)
								GCWalkContext::pushObject(context, i);
							break;
						}
						case TypeDefKind::TupleTypeDef: {
							auto td = ((TupleTypeDefObject *)typeDef);
							for (auto i : td->elementTypes)
								GCWalkContext::pushObject(context, i);
							break;
						}
						case TypeDefKind::SIMDTypeDef: {
							auto td = ((SIMDTypeDefObject *)typeDef);
							GCWalkContext::pushObject(context, td->type);
							break;
						}
						case TypeDefKind::UnpackingTypeDef: {
							auto td = ((UnpackingTypeDefObject *)typeDef);
							GCWalkContext::pushObject(context, td->type);
							break;
						}
						default:
							std::terminate();
					}
					break;
				}
				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					GCWalkContext::pushObject(context, value->_class);

					if (value->_class->cachedObjectLayout) {
						for (auto &i : value->_class->cachedObjectLayout->fieldRecords) {
							switch (i.type.typeId) {
								case TypeId::String:
								case TypeId::Instance:
								case TypeId::Array:
								case TypeId::Ref:
									GCWalkContext::pushObject(context, *((Object **)(value->rawFieldData + i.offset)));
									break;
							}
						}
					}

					GCWalkContext::removeFromDestructibleList(value);
					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gcWalk(context, value->elementType);

					switch (value->elementType.typeId) {
						case TypeId::Instance: {
							for (size_t i = 0; i < value->length; ++i)
								GCWalkContext::pushObject(context, ((Object **)value->data)[i]);
							break;
						}
					}
					break;
				}
				case ObjectKind::Module: {
					for (auto i = ((ModuleObject *)v)->members.begin(); i != ((ModuleObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(context, i.value());
					}
					for (size_t i = 0; i < ((ModuleObject *)v)->fieldRecords.size(); ++i) {
						_gcWalk(context, readVarUnsafe(EntityRef::makeStaticFieldRef((ModuleObject *)v, i)));
					}

					GCWalkContext::pushObject(context, ((ModuleObject *)v)->parent);

					for (auto i : ((ModuleObject *)v)->unnamedImports)
						GCWalkContext::pushObject(context, i);

					break;
				}
				case ObjectKind::Class: {
					for (auto i = ((ClassObject *)v)->members.begin(); i != ((ClassObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(context, i.value());
					}
					GCWalkContext::pushObject(context, ((ClassObject *)v)->parent);

					ClassObject *value = (ClassObject *)v;

					if (auto mt = value->cachedInstantiatedMethodTable; mt) {
						_gcWalk(context, mt);
					}

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, value->fieldRecords.at(i).type);
						_gcWalk(context, readVarUnsafe(EntityRef::makeStaticFieldRef(value, i)));
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
						GCWalkContext::pushObject(context, i.value());
					}
					GCWalkContext::pushObject(context, ((StructObject *)v)->parent);

					StructObject *value = (StructObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, value->fieldRecords.at(i).type);
						_gcWalk(context, readVarUnsafe(EntityRef::makeStaticFieldRef(value, i)));
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
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.
					for (auto i = ((InterfaceObject *)v)->members.begin(); i != ((InterfaceObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(context, i.value());
					}
					GCWalkContext::pushObject(context, ((InterfaceObject *)v)->parent);

					InterfaceObject *value = (InterfaceObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, readVarUnsafe(EntityRef::makeStaticFieldRef(value, i)));
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						GCWalkContext::pushObject(context, i.typeDef);
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

					GCWalkContext::pushObject(context, fn->parent);

					for (auto i : fn->overloadings) {
						GCWalkContext::pushObject(context, i.second);
					}

					break;
				}
				case ObjectKind::FnOverloading: {
					auto fnOverloading = (FnOverloadingObject *)v;

					GCWalkContext::pushObject(context, fnOverloading->fnObject);

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

					GCWalkContext::pushObject(context, (FnOverloadingObject *)value->overloading);

					if (value->resumable) {
						GCWalkContext::pushObject(context, value->resumable);

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

					GCWalkContext::pushObject(context, value->thisObject);
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

			GCWalkContext::removeFromUnwalkedList(v);

			v->gcWalkContext = nullptr;
			v->gcSpinlock.unlock();

			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, char *dataStack, size_t stackSize, MajorFrame *majorFrame) {
	GCWalkContext::pushObject(context, (FnOverloadingObject *)majorFrame->curFn);
	if (majorFrame->curCoroutine)
		GCWalkContext::pushObject(context, majorFrame->curCoroutine);

	GCWalkContext::pushObject(context, majorFrame->resumable);

	if (majorFrame->resumable) {
		size_t nRegs = majorFrame->resumable->nRegs;
		for (size_t i = 0; i < nRegs; ++i)
			_gcWalk(context, calcStackAddr(dataStack, stackSize, (majorFrame->offRegs + sizeof(Value) * i)));
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, Context &ctxt) {
	bool isWalkableObjectDetected = false;

	for (size_t i = 0; i < ctxt.majorFrames.size(); ++i) {
		MajorFrame *majorFrame = ctxt.majorFrames.at(i).get();
		_gcWalk(context, ctxt.dataStack, ctxt.stackSize, majorFrame);
	}
}

SLAKE_API void GCWalkContext::pushObject(GCWalkContext *context, Object *object) {
	if (!object)
		return;

	switch (object->gcStatus) {
		case ObjectGCStatus::Unwalked:
			object->gcStatus = ObjectGCStatus::ReadyToWalk;

			object->gcSpinlock.lock();

			if (!object->gcWalkContext) {
				object->gcWalkContext = context;
			} else {
				context = object->gcWalkContext;
			}

			context->pushWalkable(object);
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

	walkableObject->nextWalkable = walkableList;
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
		unwalkedList->prevUnwalked = walkableObject;

	walkableObject->nextUnwalked = unwalkedList;

	unwalkedList = walkableObject;
}

SLAKE_API void GCWalkContext::updateUnwalkedList(Object *deletedObject) {
	MutexGuard accessMutexGuard(accessMutex);

	if (unwalkedList) {
		if (unwalkedList == deletedObject)
			unwalkedList = deletedObject;
	}

	if (deletedObject->prevUnwalked)
		deletedObject->prevUnwalked->nextUnwalked = deletedObject->nextUnwalked;

	if (deletedObject->nextUnwalked)
		deletedObject->nextUnwalked->prevUnwalked = deletedObject->prevUnwalked;

	deletedObject->prevUnwalked = nullptr;
	deletedObject->nextUnwalked = nullptr;
}

SLAKE_API Object *GCWalkContext::getWalkedList() {
	return walkedList;
}

SLAKE_API InstanceObject *GCWalkContext::getDestructibleList() {
	MutexGuard accessMutexGuard(accessMutex);

	auto p = destructibleList;
	destructibleList = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::pushDestructible(InstanceObject *v) {
	MutexGuard accessMutexGuard(accessMutex);

	Runtime::addSameKindObjectToList((Object **)&destructibleList, v);
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
rescan:
	GCWalkContext context;

	Object *hostRefList = nullptr;

	{
		Object *prev = nullptr;

		for (Object *i = objectList; i; i = i->nextSameGenObject) {
			i->gcStatus = ObjectGCStatus::Unwalked;
			i->gcWalkContext = &context;

			i->nextWalkable = nullptr;

			i->nextHostRef = nullptr;

			i->nextUnwalked = nullptr;
			i->prevUnwalked = nullptr;

			i->objectGeneration = newGeneration;

			// Check if the object is referenced by the host, if so, exclude them into a separated list.
			if (i->hostRefCount) {
				i->nextHostRef = hostRefList;
				hostRefList = i;
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

			context.pushUnwalked(i);

			prev = i;
		}

		endObjectOut = prev;
	}

	for (Object *i = hostRefList, *next; i; i = next) {
		next = i->nextHostRef;
		GCWalkContext::pushObject(&context, i);
		i->nextHostRef = nullptr;
	}

	for (auto i : _genericCacheDir) {
		for (auto j : i.second) {
			GCWalkContext::pushObject(&context, i.first);
			GCWalkContext::pushObject(&context, j.second);
		}
	}

	for (auto i : managedThreadRunnables) {
		GCWalkContext::pushObject(&context, i.second->context.get());
	}

	if (!(_flags & _RT_DEINITING)) {
		// Walk the root node.
		GCWalkContext::pushObject(&context, _rootObject);

		// Walk contexts for each thread.
		for (auto &i : activeContexts)
			GCWalkContext::pushObject(&context, i.second);
	}

	for (Object *p = context.getWalkableList(), *i; p;) {
		i = p;

		while (i) {
			Object *next = i->nextWalkable;

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
		next = i->nextUnwalked;

		switch (curGcTarget) {
			case GCTarget::TypeDef:
				if (!isTypeDefObject(i))
					continue;
				unregisterTypeDef((TypeDefObject*)i);
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
		next = i->nextWalked;
		i->nextWalked = nullptr;
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

		for (Object *p = context.getWalkableList(), *i; p;) {
			i = p;

			while (i) {
				Object *next = i->nextWalkable;
				switch (i->gcStatus) {
					case ObjectGCStatus::Unwalked:
						std::terminate();
						break;
					case ObjectGCStatus::ReadyToWalk:
						runtime->_gcWalk(&context, i);
						break;
					case ObjectGCStatus::Walked:
						std::terminate();
						break;
				}
				i = next;
			}

			p = context.getWalkableList();
		}

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

	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		curRunnable->context.reset();
	}

	{
		Object *curObjIt = objectList, *cur = nullptr;

		size_t stepRemainder = nRecordedObjects % nMaxGcThreads;
		size_t step = nRecordedObjects / nMaxGcThreads + (stepRemainder > 0);
		for (size_t idx = 0, j = 0; j < nRecordedObjects; j += step, ++idx) {
			GCWalkContext &context = parallelGcThreadRunnables.at(idx)->context;

			Object *hostRefList = nullptr;

			{
				size_t nObj = nRecordedObjects - j;

				if (nObj > step) {
					nObj = step;
				}

				for (size_t i = 0; i < nObj; ++i) {
					cur = curObjIt;
					curObjIt = curObjIt->nextSameGenObject;

					cur->gcStatus = ObjectGCStatus::Unwalked;
					cur->gcWalkContext = &context;

					cur->nextWalkable = nullptr;

					cur->nextHostRef = nullptr;

					cur->nextUnwalked = nullptr;
					cur->prevUnwalked = nullptr;

					cur->objectGeneration = newGeneration;

					// Check if the object is referenced by the host, if so, exclude them into a separated list.
					if (cur->hostRefCount) {
						cur->nextHostRef = hostRefList;
						hostRefList = cur;
					}

					switch (cur->getObjectKind()) {
						case ObjectKind::Instance: {
							InstanceObject *value = (InstanceObject *)cur;

							if (!(value->_flags & VF_DESTRUCTED)) {
								context.pushDestructible(value);
							}
							break;
						}
						default:
							break;
					}

					context.pushUnwalked(cur);
				}
			}

			for (Object *i = hostRefList, *next; i; i = next) {
				next = i->nextHostRef;
				GCWalkContext::pushObject(&context, i);
				i->nextHostRef = nullptr;
			}
		}

		endObjectOut = cur;
	}

	for (auto &t : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)t->runnable;
		GCWalkContext &context = curRunnable->context;

		for (auto i = _genericCacheDir.begin(); i != _genericCacheDir.end(); ++i) {
			for (auto j = i.value().begin(); j != i.value().end(); ++j) {
				GCWalkContext::pushObject(&context, j.value());
			}
		}

		for (auto i : managedThreadRunnables) {
			GCWalkContext::pushObject(&context, i.second->context.get());
		}

		if (!(_flags & _RT_DEINITING)) {
			// Walk the root node.
			GCWalkContext::pushObject(&context, _rootObject);

			// Walk contexts for each thread.
			for (auto &i : activeContexts)
				GCWalkContext::pushObject(&context, i.second);
		}
	}

rescanLeftovers:
	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		curRunnable->isActive = true;

		curRunnable->activeCond.notifyAll();
	}

	for (auto &i : parallelGcThreads) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)i->runnable;

		while (!curRunnable->isDone) {
			yieldCurrentThread();
			curRunnable->doneCond.wait();
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

	size_t nDeletedObjects = 0;

	GCTarget curGcTarget = GCTarget::TypeDef;
	bool clearUnwalkedList = false;
rescanDeletables:
	for (size_t i = 0; i < parallelGcThreads.size(); ++i) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)parallelGcThreads.at(i)->runnable;
		GCWalkContext &context = curRunnable->context;

		// Delete unreachable objects.
		Object *j = context.getUnwalkedList(clearUnwalkedList);
		bool updateUnwalkedList;
		switch (curGcTarget) {
			case GCTarget::TypeDef: {
				if (!isTypeDefObject(j))
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
		for (Object *next; j; j = next) {
			next = j->nextUnwalked;

			switch (curGcTarget) {
				case GCTarget::TypeDef:
					if (!isTypeDefObject(j))
						continue;
					unregisterTypeDef((TypeDefObject *)j);
					break;
				case GCTarget::All:
					break;
				default:
					std::terminate();
			}

			if (j == objectList) {
				assert(!j->prevSameGenObject);
				objectList = j->nextSameGenObject;
			}

			if (j->prevSameGenObject) {
				j->prevSameGenObject->nextSameGenObject = j->nextSameGenObject;
			}

			if (j->nextSameGenObject) {
				j->nextSameGenObject->prevSameGenObject = j->prevSameGenObject;
			}

			context.updateUnwalkedList(j);

			if (updateUnwalkedList)
				context.updateUnwalkedList(j);

			j->dealloc();

			++nDeletedObjects;
		}
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

	if (isRescanNeeded) {
		goto rescan;
	}

	if (!nObjects) {
		assert(!objectList);
	}

	for (Object *i = objectList; i; i = i->nextSameGenObject) {
		i->gcStatus = ObjectGCStatus::Unwalked;
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

		if (!(p = std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>>(peff::allocAndConstruct<ParallelGcThreadRunnable>(getFixedAlloc(), alignof(ParallelGcThreadRunnable), this)))) {
			return false;
		}
	}

	if (!parallelGcThreads.resize(nMaxGcThreads)) {
		return false;
	}

	for (size_t i = 0; i < nMaxGcThreads; ++i) {
		if (!(parallelGcThreads.at(i) = std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>>(Thread::alloc(getFixedAlloc(), parallelGcThreadRunnables.at(i).get(), 4096)))) {
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

		i->activeCond.notifyAll();

		while (i->threadState == ParallelGcThreadState::NotifyTermination)
			yieldCurrentThread();
	}

	parallelGcThreads.clear();
	parallelGcThreadRunnables.clear();
}
