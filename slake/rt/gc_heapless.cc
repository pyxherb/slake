#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto i = methodTable->methods.begin(); i != methodTable->methods.end(); ++i) {
		GCWalkContext::pushObject(context, i.value());
	}
	for (auto i : methodTable->destructors) {
		GCWalkContext::pushObject(context, i);
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, GenericParamList &genericParamList) {
	for (auto &i : genericParamList) {
		// i.baseType.loadDeferredType(this);
		if (auto p = i.baseType.resolveCustomType(); p)
			GCWalkContext::pushObject(context, p);

		for (auto &j : i.interfaces) {
			// j.loadDeferredType(this);
			if (auto p = j.resolveCustomType(); p)
				GCWalkContext::pushObject(context, p);
		}
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, const Type &type) {
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
			GCWalkContext::pushObject(context, type.getCustomTypeExData());
			break;
		case TypeId::GenericArg:
			GCWalkContext::pushObject(context, type.exData.genericArg.ownerObject);
			GCWalkContext::pushObject(context, type.exData.genericArg.nameObject);
			break;
		case TypeId::Array:
			if (type.exData.typeDef)
				GCWalkContext::pushObject(context, type.exData.typeDef);
			break;
		case TypeId::Ref:
			if (type.exData.typeDef)
				GCWalkContext::pushObject(context, type.exData.typeDef);
			break;
		case TypeId::None:
		case TypeId::Any:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
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
				case ObjectRefKind::FieldRef:
					GCWalkContext::pushObject(context, entityRef.asField.moduleObject);
					break;
				case ObjectRefKind::ArrayElementRef:
					GCWalkContext::pushObject(context, entityRef.asArray.arrayObject);
					break;
				case ObjectRefKind::ObjectRef:
					GCWalkContext::pushObject(context, entityRef.asObject.instanceObject);
					break;
				case ObjectRefKind::InstanceFieldRef:
					GCWalkContext::pushObject(context, entityRef.asObjectField.instanceObject);
					break;
				case ObjectRefKind::LocalVarRef:
					_gcWalk(context, *entityRef.asLocalVar.context);
					_gcWalk(context, readVarUnsafe(entityRef));
					break;
				case ObjectRefKind::CoroutineLocalVarRef:
					GCWalkContext::pushObject(context, entityRef.asCoroutineLocalVar.coroutine);
					_gcWalk(context, readVarUnsafe(entityRef));
					break;
				case ObjectRefKind::ArgRef:
					break;
				case ObjectRefKind::CoroutineArgRef:
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

			switch (auto typeId = v->objectKind; typeId) {
				case ObjectKind::String:
					break;
				case ObjectKind::TypeDef:
					_gcWalk(context, ((TypeDefObject *)v)->type);
					break;
				case ObjectKind::FnTypeDef: {
					auto typeDef = ((FnTypeDefObject *)v);
					_gcWalk(context, typeDef->returnType);
					for (auto &i : typeDef->paramTypes)
						_gcWalk(context, i);
					break;
				}
				case ObjectKind::ParamTypeListTypeDef: {
					auto typeDef = ((ParamTypeListTypeDefObject *)v);
					for (auto &i : typeDef->paramTypes)
						_gcWalk(context, i);
					break;
				}

				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					GCWalkContext::pushObject(context, value->_class);
					if (auto mt = value->_class->cachedInstantiatedMethodTable) {
						_gcWalk(context, mt);
					}

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
						_gcWalk(context, readVarUnsafe(EntityRef::makeFieldRef((ModuleObject *)v, i)));
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

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, value->fieldRecords.at(i).type);
						_gcWalk(context, readVarUnsafe(EntityRef::makeFieldRef(value, i)));
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
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.
					for (auto i = ((InterfaceObject *)v)->members.begin(); i != ((InterfaceObject *)v)->members.end(); ++i) {
						GCWalkContext::pushObject(context, i.value());
					}
					GCWalkContext::pushObject(context, ((InterfaceObject *)v)->parent);

					InterfaceObject *value = (InterfaceObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalk(context, readVarUnsafe(EntityRef::makeFieldRef(value, i)));
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						GCWalkContext::pushObject(context, i.getCustomTypeExData());
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
						GCWalkContext::pushObject(context, i);
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

					if (value->paramTypes) {
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
								_gcWalk(context, *((Value *)(value->stackData + value->lenStackData - (value->offStackTop + sizeof(Value) * i))));
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
						for (auto &l : k.exceptHandlers) {
							_gcWalk(context, l.type);
						}
					}

					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}

			GCWalkContext::removeFromUnwalkedList(v);

			v->gcWalkContext = nullptr;
			v->gcMutex.unlock();

			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, char *dataStack, MajorFrame *majorFrame) {
	GCWalkContext::pushObject(context, (FnOverloadingObject *)majorFrame->curFn);
	if (majorFrame->curCoroutine)
		GCWalkContext::pushObject(context, majorFrame->curCoroutine);

	GCWalkContext::pushObject(context, majorFrame->resumable);

	if (majorFrame->resumable) {
		size_t nRegs = majorFrame->resumable->nRegs;
		for (size_t i = 0; i < nRegs; ++i)
			_gcWalk(context, *((Value *)(dataStack + SLAKE_STACK_MAX - (majorFrame->offRegs + sizeof(Value) * i))));
	}
}

SLAKE_API void Runtime::_gcWalk(GCWalkContext *context, Context &ctxt) {
	bool isWalkableObjectDetected = false;

	for (size_t i = 0; i < ctxt.majorFrames.size(); ++i) {
		MajorFrame *majorFrame = ctxt.majorFrames.at(i).get();
		_gcWalk(context, ctxt.dataStack, majorFrame);
	}
}

SLAKE_API void GCWalkContext::pushObject(GCWalkContext *context, Object *object) {
	if (!object)
		return;

	switch (object->gcStatus) {
		case ObjectGCStatus::Unwalked:
			object->gcStatus = ObjectGCStatus::ReadyToWalk;

			object->gcMutex.lock();

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

SLAKE_API Object *GCWalkContext::getUnwalkedList() {
	MutexGuard accessMutexGuard(accessMutex);

	Object *p = unwalkedList;
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

			switch (i->objectKind) {
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

			prev = i;
		}

		endObjectOut = prev;
	}

	for (Object *i = hostRefList, *next; i; i = next) {
		next = i->nextHostRef;
		GCWalkContext::pushObject(&context, i);
		i->nextHostRef = nullptr;
	}

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

	if (InstanceObject *p = context.getDestructibleList(); p) {
		_destructDestructibleObjects(p);
		isRescanNeeded = true;
	}

	if (isRescanNeeded) {
		goto rescan;
	}

	size_t nDeletedObjects = 0;
	// Delete unreachable objects.
	for (Object *i = context.getUnwalkedList(), *next; i; i = next) {
		next = i->nextUnwalked;

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

		i->dealloc();

		++nDeletedObjects;
	}

	nObjects -= nDeletedObjects;

	for (Object *i = objectList; i; i = i->nextSameGenObject) {
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

					switch (cur->objectKind) {
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

	for (size_t i = 0; i < parallelGcThreads.size(); ++i) {
		ParallelGcThreadRunnable *curRunnable = (ParallelGcThreadRunnable *)parallelGcThreads.at(i)->runnable;
		GCWalkContext &context = curRunnable->context;

		// Delete unreachable objects.
		for (Object *j = context.getUnwalkedList(), *next; j; j = next) {
			next = j->nextUnwalked;

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

			j->dealloc();

			++nDeletedObjects;
		}
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
		if (!(parallelGcThreads.at(i) = std::unique_ptr<Thread, util::DeallocableDeleter<Thread>>(Thread::alloc(getFixedAlloc(), parallelGcThreadRunnables.at(i).get(), 4096)))) {
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
