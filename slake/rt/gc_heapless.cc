#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto i = methodTable->methods.begin(); i != methodTable->methods.end(); ++i) {
		context.pushObject(i.value());
	}
	for (auto i : methodTable->destructors) {
		context.pushObject(i);
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, GenericParamList &genericParamList) {
	for (auto &i : genericParamList) {
		// i.baseType.loadDeferredType(this);
		if (auto p = i.baseType.resolveCustomType(); p)
			context.pushObject(p);

		for (auto &j : i.interfaces) {
			// j.loadDeferredType(this);
			if (auto p = j.resolveCustomType(); p)
				context.pushObject(p);
		}
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, const Type &type) {
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
		case TypeId::GenericArg:
			context.pushObject(type.getCustomTypeExData());
			break;
		case TypeId::Array:
			if (type.exData.typeDef)
				context.pushObject(type.exData.typeDef);
			break;
		case TypeId::Ref:
			if (type.exData.typeDef)
				context.pushObject(type.exData.typeDef);
			break;
		case TypeId::None:
		case TypeId::Any:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, const Value &i) {
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
					context.pushObject(entityRef.asField.moduleObject);
					break;
				case ObjectRefKind::ArrayElementRef:
					context.pushObject(entityRef.asArray.arrayObject);
					break;
				case ObjectRefKind::ObjectRef:
					context.pushObject(entityRef.asObject.instanceObject);
					break;
				case ObjectRefKind::InstanceFieldRef:
					context.pushObject(entityRef.asObjectField.instanceObject);
					break;
				case ObjectRefKind::LocalVarRef:
					context.pushObject(entityRef.asCoroutineLocalVar.coroutine);
					_gcWalkHeapless(context, readVarUnsafe(entityRef));
					break;
				case ObjectRefKind::CoroutineLocalVarRef:
					_gcWalkHeapless(context, readVarUnsafe(entityRef));
					break;
				case ObjectRefKind::ArgRef:
					break;
				case ObjectRefKind::CoroutineArgRef:
					context.pushObject(entityRef.asCoroutineArg.coroutine);
					break;
			}
			break;
		}
		case ValueType::RegRef:
			break;
		case ValueType::TypeName:
			_gcWalkHeapless(context, i.getTypeName());
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, Object *v) {
	if (!v)
		return;

	if (v->gcInfo.heapless.nextUnwalked) {
		if (v == context.unwalkedList) {
			context.unwalkedList = v->gcInfo.heapless.nextUnwalked;
		}
		v->gcInfo.heapless.nextUnwalked->gcInfo.heapless.prevUnwalked = v->gcInfo.heapless.prevUnwalked;
	}
	if (v->gcInfo.heapless.prevUnwalked) {
		if (v == context.unwalkedList) {
			context.unwalkedList = v->gcInfo.heapless.prevUnwalked;
		}
		v->gcInfo.heapless.prevUnwalked->gcInfo.heapless.nextUnwalked = v->gcInfo.heapless.nextUnwalked;
	}

	if (v == destructibleList) {
		destructibleList = v->gcInfo.heapless.nextDestructible;
	}
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

	switch (v->gcInfo.heapless.gcStatus) {
		case ObjectGCStatus::Unwalked:
			throw std::logic_error("Cannot walk on an unwalked object");
		case ObjectGCStatus::ReadyToWalk:
			v->gcInfo.heapless.gcStatus = ObjectGCStatus::Walked;
			switch (auto typeId = v->getKind(); typeId) {
				case ObjectKind::String:
					break;
				case ObjectKind::TypeDef:
					_gcWalkHeapless(context, ((TypeDefObject *)v)->type);
					break;
				case ObjectKind::FnTypeDef: {
					auto typeDef = ((FnTypeDefObject *)v);
					_gcWalkHeapless(context, typeDef->returnType);
					for (auto &i : typeDef->paramTypes)
						_gcWalkHeapless(context, i);
					break;
				}
				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					context.pushObject(value->_class);
					if (auto mt = value->_class->cachedInstantiatedMethodTable) {
						_gcWalkHeapless(context, mt);
					}

					if (value->_class->cachedObjectLayout) {
						for (auto &i : value->_class->cachedObjectLayout->fieldRecords) {
							_gcWalkHeapless(context, i.type);

							switch (i.type.typeId) {
								case TypeId::String:
								case TypeId::Instance:
								case TypeId::Array:
								case TypeId::Ref:
									context.pushObject(*((Object **)(value->rawFieldData + i.offset)));
									break;
							}
						}
					}

					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gcWalkHeapless(context, value->elementType);

					switch (value->elementType.typeId) {
						case TypeId::Instance: {
							for (size_t i = 0; i < value->length; ++i)
								context.pushObject(((Object **)value->data)[i]);
							break;
						}
					}
					break;
				}
				case ObjectKind::Module: {
					for (auto i = ((ModuleObject *)v)->members.begin(); i != ((ModuleObject *)v)->members.end(); ++i) {
						context.pushObject(i.value());
					}
					for (size_t i = 0; i < ((ModuleObject *)v)->fieldRecords.size(); ++i) {
						_gcWalkHeapless(context, readVarUnsafe(EntityRef::makeFieldRef((ModuleObject *)v, i)));
					}

					context.pushObject(((ModuleObject *)v)->parent);

					for (auto i : ((ModuleObject *)v)->unnamedImports)
						context.pushObject(i);

					break;
				}
				case ObjectKind::Class: {
					for (auto i = ((ClassObject *)v)->members.begin(); i != ((ClassObject *)v)->members.end(); ++i) {
						context.pushObject(i.value());
					}
					context.pushObject(((ClassObject *)v)->parent);

					ClassObject *value = (ClassObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalkHeapless(context, readVarUnsafe(EntityRef::makeFieldRef(value, i)));
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						_gcWalkHeapless(context, i);
					}
					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalkHeapless(context, i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalkHeapless(context, j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalkHeapless(context, i);
					}

					// value->parentClass.loadDeferredType(this);
					_gcWalkHeapless(context, value->baseType);

					_gcWalkHeapless(context, value->genericParams);

					break;
				}
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.
					for (auto i = ((InterfaceObject *)v)->members.begin(); i != ((InterfaceObject *)v)->members.end(); ++i) {
						context.pushObject(i.value());
					}
					context.pushObject(((InterfaceObject *)v)->parent);

					InterfaceObject *value = (InterfaceObject *)v;

					for (size_t i = 0; i < value->fieldRecords.size(); ++i) {
						_gcWalkHeapless(context, readVarUnsafe(EntityRef::makeFieldRef(value, i)));
					}

					for (auto &i : value->implTypes) {
						// i.loadDeferredType(this);
						context.pushObject(i.getCustomTypeExData());
					}
					for (auto &i : value->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalkHeapless(context, i.baseType);
						for (auto &j : i.interfaces) {
							_gcWalkHeapless(context, j);
						}
					}
					for (auto &i : value->genericArgs) {
						// i.loadDeferredType(this);
						_gcWalkHeapless(context, i);
					}

					_gcWalkHeapless(context, value->genericParams);

					break;
				}
				case ObjectKind::Fn: {
					auto fn = (FnObject *)v;

					context.pushObject(fn->parent);

					for (auto i : fn->overloadings) {
						context.pushObject(i);
					}
					break;
				}
				case ObjectKind::FnOverloading: {
					auto fnOverloading = (FnOverloadingObject *)v;

					context.pushObject(fnOverloading->fnObject);

					for (auto &i : fnOverloading->genericParams) {
						// i.baseType.loadDeferredType(this);
						_gcWalkHeapless(context, i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalkHeapless(context, j);
						}
					}
					for (auto i = fnOverloading->mappedGenericArgs.begin(); i != fnOverloading->mappedGenericArgs.end(); ++i) {
						// i.value().loadDeferredType(this);
						_gcWalkHeapless(context, i.value());
					}

					for (auto &j : fnOverloading->paramTypes)
						_gcWalkHeapless(context, j);
					_gcWalkHeapless(context, fnOverloading->returnType);

					switch (fnOverloading->overloadingKind) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fnOverloading;

							for (auto &i : ol->instructions) {
								for (size_t j = 0; j < i.nOperands; ++j) {
									_gcWalkHeapless(context, i.operands[j]);
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

					_gcWalkHeapless(context, fnOverloading->genericParams);

					break;
				}
				case ObjectKind::IdRef: {
					auto value = (IdRefObject *)v;

					for (auto &i : value->entries) {
						for (auto &j : i.genericArgs) {
							_gcWalkHeapless(context, j);
						}
					}

					if (value->paramTypes) {
						for (auto &j : *value->paramTypes) {
							_gcWalkHeapless(context, j);
						}
					}
					break;
				}
				case ObjectKind::Context: {
					auto value = (ContextObject *)v;

					_gcWalkHeapless(context, value->_context);
					break;
				}
				case ObjectKind::Coroutine: {
					auto value = (CoroutineObject *)v;

					context.pushObject((FnOverloadingObject *)value->overloading);
					context.pushObject(value->resumable.thisObject);
					for (auto &k : value->resumable.argStack) {
						_gcWalkHeapless(context, k.type);
						_gcWalkHeapless(context, k.value);
					}
					for (auto &k : value->resumable.nextArgStack)
						_gcWalkHeapless(context, k);
					for (auto &k : value->resumable.minorFrames) {
						for (auto &l : k.exceptHandlers) {
							_gcWalkHeapless(context, l.type);
						}
					}
					if (value->stackData) {
						for (size_t i = 0; i < value->resumable.nRegs; ++i)
							_gcWalkHeapless(context, *((Value *)(value->stackData + value->lenStackData - (value->offStackTop + sizeof(Value) * i))));
					}
					if (value->isDone()) {
						_gcWalkHeapless(context, value->finalResult);
					}
					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}
			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, char *dataStack, MajorFrame *majorFrame) {
	context.pushObject((FnOverloadingObject *)majorFrame->curFn);
	if (majorFrame->curCoroutine)
		context.pushObject(majorFrame->curCoroutine);
	context.pushObject(majorFrame->resumable.thisObject);
	_gcWalkHeapless(context, majorFrame->curExcept);
	for (auto &k : majorFrame->resumable.argStack) {
		_gcWalkHeapless(context, k.type);
		_gcWalkHeapless(context, k.value);
	}
	for (auto &k : majorFrame->resumable.nextArgStack)
		_gcWalkHeapless(context, k);
	for (size_t i = 0; i < majorFrame->resumable.nRegs; ++i)
		_gcWalkHeapless(context, *((Value *)(dataStack + SLAKE_STACK_MAX - (majorFrame->offRegs + sizeof(Value) * i))));
	for (auto &k : majorFrame->resumable.minorFrames) {
		for (auto &l : k.exceptHandlers)
			_gcWalkHeapless(context, l.type);
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, Context &ctxt) {
	bool isWalkableObjectDetected = false;
	for (auto &i : ctxt.majorFrames) {
		MajorFrame *majorFrame = i.get();
		_gcWalkHeapless(context, majorFrame);
	}
}

SLAKE_API void Runtime::GCHeaplessWalkContext::pushObject(Object *object) {
	if (!object)
		return;
	switch (object->gcInfo.heapless.gcStatus) {
		case ObjectGCStatus::Unwalked:
			object->gcInfo.heapless.gcStatus = ObjectGCStatus::ReadyToWalk;
			object->gcInfo.heapless.nextWalkable = walkableList;

			walkableList = object;
			break;
		case ObjectGCStatus::ReadyToWalk:
			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcHeapless() {
rescan:
	GCHeaplessWalkContext context;

	{
		Object *lastObject = nullptr;
		InstanceObject *lastDestructibleInstanceObject = nullptr;
		for (auto i : createdObjects) {
			i->gcInfo.heapless.gcStatus = ObjectGCStatus::Unwalked;
			i->gcInfo.heapless.nextWalkable = nullptr;

			// Check if the object is referenced by the host, if so, exclude them into a separated list.
			if (i->hostRefCount) {
				i->gcInfo.heapless.nextHostRef = context.hostRefList;
				context.hostRefList = i;
			}

			switch (i->getKind()) {
				case ObjectKind::Instance: {
					InstanceObject *value = (InstanceObject *)i;

					if (!(value->_flags & VF_DESTRUCTED)) {
						i->gcInfo.heapless.nextDestructible = lastDestructibleInstanceObject;
						i->gcInfo.heapless.prevDestructible = nullptr;

						if (lastDestructibleInstanceObject) {
							lastDestructibleInstanceObject->gcInfo.heapless.prevDestructible = lastDestructibleInstanceObject;
						}

						context.destructibleList = value;

						lastDestructibleInstanceObject = value;
					} else {
						i->gcInfo.heapless.nextDestructible = nullptr;
						i->gcInfo.heapless.prevDestructible = nullptr;
					}
					break;
				}
				default:
					i->gcInfo.heapless.nextDestructible = nullptr;
					i->gcInfo.heapless.prevDestructible = nullptr;
					break;
			}

			{
				i->gcInfo.heapless.nextUnwalked = lastObject;
				i->gcInfo.heapless.prevUnwalked = nullptr;
				if (lastObject) {
					lastObject->gcInfo.heapless.prevUnwalked = i;
				}

				context.unwalkedList = i;
			}

			lastObject = i;
		}
	}

	for (Object *i = context.hostRefList, *next; i; i = next) {
		next = i->gcInfo.heapless.nextHostRef;
		context.pushObject(i);
		i->gcInfo.heapless.nextHostRef = nullptr;
	}

	for (auto i = _genericCacheDir.begin(); i != _genericCacheDir.end(); ++i) {
		for (auto j = i.value().begin(); j != i.value().end(); ++j) {
			context.pushObject(j.value());
		}
	}

	for (auto &i : managedThreadRunnables) {
		context.pushObject(i.second->context.get());
	}

	if (!(_flags & _RT_DEINITING)) {
		// Walk the root node.
		context.pushObject(_rootObject);

		// Walk contexts for each thread.
		for (auto &i : activeContexts)
			context.pushObject(i.second);
	}

	while (context.walkableList) {
		Object *i = context.walkableList;
		context.walkableList = nullptr;

		while (i) {
			Object *next = i->gcInfo.heapless.nextWalkable;
			switch (i->gcInfo.heapless.gcStatus) {
				case ObjectGCStatus::Unwalked:
					assert(false);
					break;
				case ObjectGCStatus::ReadyToWalk:
					_gcWalkHeapless(context, i);
					break;
				case ObjectGCStatus::Walked:
					assert(false);
					break;
			}
			i = next;
		}
	}

	if ((destructibleList = context.destructibleList)) {
		_destructDestructibleObjects();
		goto rescan;
	}

	// Delete unreachable objects.
	for (Object *i = context.unwalkedList, *next; i; i = next) {
		next = i->gcInfo.heapless.nextUnwalked;

		i->dealloc();
		createdObjects.remove(i);
	}
}
