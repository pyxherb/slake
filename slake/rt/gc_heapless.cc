#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::GCHeaplessWalkContext::pushObject(Object *object) {
	if (!object)
		return;
	switch (object->gcInfo.heapless.gcStatus) {
		case ObjectGCStatus::Unwalked:
			object->gcInfo.heapless.gcStatus = ObjectGCStatus::ReadyToWalk;
			object->gcInfo.heapless.next = walkableList;
			walkableList = object;
			break;
		case ObjectGCStatus::ReadyToWalk:
			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, Scope *scope) {
	if (!scope)
		return;
	for (auto &i : scope->members) {
		context.pushObject(i.second);
	}

	if (scope->owner)
		context.pushObject(scope->owner);
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto &i : methodTable->methods) {
		context.pushObject(i.second);
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
		case TypeId::Value:
		case TypeId::String:
			break;
		case TypeId::Instance:
		case TypeId::GenericArg:
			context.pushObject(type.getCustomTypeExData());
			break;
		case TypeId::Array:
			if (type.exData.ptr)
				context.pushObject(type.exData.ptr);
			break;
		case TypeId::Ref:
			if (type.exData.ptr)
				context.pushObject(type.exData.ptr);
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
		case ValueType::ObjectRef:
			context.pushObject(i.getObjectRef());
			break;
		case ValueType::RegRef:
			break;
		case ValueType::TypeName:
			_gcWalkHeapless(context, i.getTypeName());
			break;
		case ValueType::VarRef: {
			auto &varRef = i.getVarRef();

			context.pushObject(varRef.varPtr);
			break;
		}
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, Object *v) {
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
					_gcWalkHeapless(context, value->methodTable);

					for (auto &i : value->objectLayout->fieldRecords) {
						_gcWalkHeapless(context, i.type);

						switch (i.type.typeId) {
							case TypeId::Value: {
								switch (i.type.getValueTypeExData()) {
									case ValueType::ObjectRef:
										context.pushObject(*((Object **)(value->rawFieldData + i.offset)));
										break;
								}
								break;
							}
							case TypeId::String:
							case TypeId::Instance:
							case TypeId::Array:
							case TypeId::Ref:
								context.pushObject(*((Object **)(value->rawFieldData + i.offset)));
								break;
						}
					}
					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gcWalkHeapless(context, value->elementType);
					context.pushObject(value->accessor);

					switch (value->elementType.typeId) {
						case TypeId::Instance: {
							auto v = (ObjectRefArrayObject *)value;
							for (size_t i = 0; i < v->length; ++i)
								context.pushObject(v->data[i]);
							break;
						}
					}
					break;
				}
				case ObjectKind::Module:
				case ObjectKind::Class:
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.

					_gcWalkHeapless(context, ((ModuleObject *)v)->scope);
					context.pushObject(((ModuleObject *)v)->parent);

					for (auto &i : ((ModuleObject *)v)->imports)
						context.pushObject(i.second);

					switch (typeId) {
						case ObjectKind::Class: {
							ClassObject *value = (ClassObject *)v;
							for (auto &i : value->implInterfaces) {
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
							context.pushObject(value->parentClass.resolveCustomType());

							_gcWalkHeapless(context, value->genericParams);
							break;
						}
						case ObjectKind::Interface: {
							InterfaceObject *value = (InterfaceObject *)v;

							for (auto &i : value->parents) {
								// i.loadDeferredType(this);
								context.pushObject(i.getCustomTypeExData());
							}
							for (auto &i : value->genericParams) {
								// i.baseType.loadDeferredType(this);
								_gcWalkHeapless(context, i.baseType);
								for (auto &j : i.interfaces) {
									j.loadDeferredType(this);
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
					}

					break;
				}
				case ObjectKind::Var: {
					VarObject *value = (VarObject *)v;

					switch (value->getVarKind()) {
						case VarKind::Regular: {
							auto v = (RegularVarObject *)value;

							Value data;
							InternalExceptionPointer result = value->getData(VarRefContext(), data);
							assert(!result);
							_gcWalkHeapless(context, data);

							context.pushObject(v->parent);

							_gcWalkHeapless(context, v->type);
							break;
						}
						case VarKind::ArrayElementAccessor: {
							auto v = (ArrayAccessorVarObject *)value;

							context.pushObject(v->arrayObject);
							break;
						}
						case VarKind::InstanceMemberAccessor: {
							auto v = (InstanceMemberAccessorVarObject *)value;

							context.pushObject(v->instanceObject);
							break;
						}
						case VarKind::LocalVarAccessor: {
							auto v = (LocalVarAccessorVarObject *)value;

							break;
						}
					}
					break;
				}
				case ObjectKind::RootObject: {
					RootObject *value = (RootObject *)v;
					_gcWalkHeapless(context, value->scope);
					break;
				}
				case ObjectKind::Fn: {
					auto fn = (FnObject *)v;

					context.pushObject(fn->getParent());

					for (auto &i : fn->overloadings) {
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
					for (auto &i : fnOverloading->mappedGenericArgs) {
						// i.second.loadDeferredType(this);
						_gcWalkHeapless(context, i.second);
					}

					for (auto &j : fnOverloading->paramTypes)
						_gcWalkHeapless(context, j);
					_gcWalkHeapless(context, fnOverloading->returnType);

					switch (fnOverloading->getOverloadingKind()) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fnOverloading;

							for (auto &i : ol->instructions) {
								for (auto &j : i.operands) {
									_gcWalkHeapless(context, j);
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
						for (auto &j : i.paramTypes) {
							_gcWalkHeapless(context, j);
						}
					}
					break;
				}
				case ObjectKind::Alias: {
					auto value = (AliasObject *)v;

					context.pushObject(value->src);
					break;
				}
				case ObjectKind::Context: {
					auto value = (ContextObject *)v;

					_gcWalkHeapless(context, value->_context);
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

SLAKE_API void Runtime::_gcWalkHeapless(GCHeaplessWalkContext &context, Context &ctxt) {
	bool isWalkableObjectDetected = false;
	for (auto &j : ctxt.majorFrames) {
		context.pushObject((FnOverloadingObject *)j->curFn);
		_gcWalkHeapless(context, j->returnValue);
		context.pushObject(j->thisObject);
		_gcWalkHeapless(context, j->curExcept);
		for (auto &k : j->argStack)
			context.pushObject((VarObject *)k);
		for (auto &k : j->nextArgStack)
			_gcWalkHeapless(context, k);
		for (auto &k : j->localVarRecords) {
			_gcWalkHeapless(context, k.type);

			switch (k.type.typeId) {
				case TypeId::Value: {
					switch (k.type.getValueTypeExData()) {
						case ValueType::ObjectRef:
							context.pushObject(*((Object **)(ctxt.dataStack + k.stackOffset)));
							break;
					}
					break;
				}
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Ref:
					context.pushObject(*((Object **)(ctxt.dataStack + k.stackOffset)));
					break;
			}
		}
		context.pushObject(j->localVarAccessor);
		for (auto &k : j->regs)
			_gcWalkHeapless(context, k);
		for (auto &k : j->minorFrames) {
			for (auto &l : k.exceptHandlers)
				_gcWalkHeapless(context, l.type);
		}
	}
}

SLAKE_API void Runtime::_gcHeapless() {
	bool foundDestructibleObjects = false;

	GCHeaplessWalkContext context;

rescan:
	for (auto i : createdObjects) {
		i->_flags |= VF_GCREADY;
		i->gcInfo.heapless.gcStatus = ObjectGCStatus::Unwalked;
		i->gcInfo.heapless.next = nullptr;
		if (i->hostRefCount) {
			context.pushObject(i);
		}
	}

	if (!(_flags & _RT_DEINITING)) {
		// Walk the root node.
		context.pushObject(_rootObject);

		// Walk contexts for each thread.
		for (auto &i : activeContexts)
			context.pushObject(i.second);

		while (context.walkableList) {
			Object *i = context.walkableList;
			context.walkableList = nullptr;

			while (i) {
				Object *next = i->gcInfo.heapless.next;
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
				i->gcInfo.heapless.next = nullptr;
				i = next;
			}
		}
	}

	// Execute destructors for all destructible unreachable objects.
	destructingThreads.insert(std::this_thread::get_id());
	for (auto i : createdObjects) {
		switch (i->gcInfo.heapless.gcStatus) {
			case ObjectGCStatus::Unwalked:
				break;
			case ObjectGCStatus::ReadyToWalk:
				assert(false);
				break;
			case ObjectGCStatus::Walked:
				continue;
		}

		if (i->getKind() == ObjectKind::Instance) {
			InstanceObject *object = (InstanceObject *)i;

			if (auto mt = object->methodTable; mt) {
				if (mt->destructors.size()) {
					for (auto j : mt->destructors) {
						HostRefHolder holder;
						HostObjectRef<ContextObject> contextOut;
						execFn(j, nullptr, i, nullptr, 0, contextOut);
					}
					foundDestructibleObjects = true;
				}
			}
		}
	}
	destructingThreads.erase(std::this_thread::get_id());

	// Delete unreachable objects.
	for (auto it = createdObjects.begin(); it != createdObjects.end();) {
		Object *i = *it;
		switch (i->gcInfo.heapless.gcStatus) {
			case ObjectGCStatus::Unwalked:
				if ((*it)->_flags & VF_GCREADY) {
					(*it)->dealloc();
					createdObjects.erase(it++);
					continue;
				}
				break;
			case ObjectGCStatus::ReadyToWalk:
				assert(false);
				break;
			case ObjectGCStatus::Walked:
				i->gcInfo.heapless.gcStatus = ObjectGCStatus::Unwalked;
				break;
		}

		++it;
	}

	if (foundDestructibleObjects) {
		foundDestructibleObjects = false;
		goto rescan;
	}
}
