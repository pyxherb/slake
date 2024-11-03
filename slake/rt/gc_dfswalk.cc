#include "../runtime.h"

using namespace slake;

SLAKE_API bool Runtime::GCDfsWalkContext::pushObject(Object* object) {
	if (!object)
		return true;

	switch (object->gcInfo.dfs.gcStatus) {
		case ObjectGCStatus::Unwalked:
			break;
		case ObjectGCStatus::ReadyToWalk:
			assert(false);
			break;
		case ObjectGCStatus::Walked:
			return true;
	}
	objectStack.insert(object);
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, Scope *scope) {
	if (!scope)
		return true;
	for (auto &i : scope->members) {
		context.pushObject(i.second);
	}

	if (scope->owner)
		context.pushObject(scope->owner);
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, MethodTable *methodTable) {
	if (!methodTable)
		return true;
	for (auto &i : methodTable->methods) {
		context.pushObject(i.second);
	}
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, GenericParamList &genericParamList) {
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
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, const Type &type) {
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
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, const Value &i) {
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
			_gcWalkDfs(context, i.getTypeName());
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
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, Object *v) {
	if (!v)
		return true;

	switch (v->gcInfo.dfs.gcStatus) {
		case ObjectGCStatus::Unwalked:
			v->gcInfo.dfs.gcStatus = ObjectGCStatus::Walked;
			switch (auto typeId = v->getKind(); typeId) {
				case ObjectKind::String:
					break;
				case ObjectKind::TypeDef:
					_gcWalkDfs(context, ((TypeDefObject *)v)->type);
					break;
				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					context.pushObject(value->_class);
					_gcWalkDfs(context, value->methodTable);

					for (auto &i : value->objectLayout->fieldRecords) {
						_gcWalkDfs(context, i.type);

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

					context.pushObject(value->_class);
					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gcWalkDfs(context, value->elementType);
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

					_gcWalkDfs(context, ((ModuleObject *)v)->scope);
					context.pushObject(((ModuleObject *)v)->parent);

					for (auto &i : ((ModuleObject *)v)->imports)
						context.pushObject(i.second);

					switch (typeId) {
						case ObjectKind::Class: {
							ClassObject *value = (ClassObject *)v;
							for (auto &i : value->implInterfaces) {
								// i.loadDeferredType(this);
								_gcWalkDfs(context, i);
							}
							for (auto &i : value->genericParams) {
								// i.baseType.loadDeferredType(this);
								_gcWalkDfs(context, i.baseType);
								for (auto &j : i.interfaces) {
									// j.loadDeferredType(this);
									_gcWalkDfs(context, j);
								}
							}
							for (auto &i : value->genericArgs) {
								// i.loadDeferredType(this);
								_gcWalkDfs(context, i);
							}

							// value->parentClass.loadDeferredType(this);
							context.pushObject(value->parentClass.resolveCustomType());

							_gcWalkDfs(context, value->genericParams);
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
								_gcWalkDfs(context, i.baseType);
								for (auto &j : i.interfaces) {
									j.loadDeferredType(this);
									_gcWalkDfs(context, j);
								}
							}
							for (auto &i : value->genericArgs) {
								// i.loadDeferredType(this);
								_gcWalkDfs(context, i);
							}

							_gcWalkDfs(context, value->genericParams);
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
							_gcWalkDfs(context, data);

							context.pushObject(v->parent);

							_gcWalkDfs(context, v->type);
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
					_gcWalkDfs(context, value->scope);
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
						_gcWalkDfs(context, i.baseType);
						for (auto &j : i.interfaces) {
							// j.loadDeferredType(this);
							_gcWalkDfs(context, j);
						}
					}
					for (auto &i : fnOverloading->mappedGenericArgs) {
						// i.second.loadDeferredType(this);
						_gcWalkDfs(context, i.second);
					}

					for (auto &j : fnOverloading->paramTypes)
						_gcWalkDfs(context, j);
					_gcWalkDfs(context, fnOverloading->returnType);

					switch (fnOverloading->getOverloadingKind()) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fnOverloading;

							for (auto &i : ol->instructions) {
								for (auto &j : i.operands) {
									_gcWalkDfs(context, j);
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

					_gcWalkDfs(context, fnOverloading->genericParams);

					break;
				}
				case ObjectKind::IdRef: {
					auto value = (IdRefObject *)v;

					for (auto &i : value->entries) {
						for (auto &j : i.genericArgs) {
							_gcWalkDfs(context, j);
						}
						for (auto &j : i.paramTypes) {
							_gcWalkDfs(context, j);
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

					_gcWalkDfs(context, value->_context);
					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}
			break;
		case ObjectGCStatus::ReadyToWalk:
			assert(false);
			break;
		case ObjectGCStatus::Walked:
			break;
	}
	return true;
}

SLAKE_API bool Runtime::_gcWalkDfs(GCDfsWalkContext &context, Context &ctxt) {
	for (auto &j : ctxt.majorFrames) {
		context.pushObject((FnOverloadingObject *)j->curFn);
		_gcWalkDfs(context, j->returnValue);
		_gcWalkDfs(context, j->thisObject);
		_gcWalkDfs(context, j->curExcept);
		for (auto &k : j->argStack)
			context.pushObject((VarObject *)k);
		for (auto &k : j->nextArgStack)
			_gcWalkDfs(context, k);
		for (auto &k : j->localVarRecords) {
			_gcWalkDfs(context, k.type);

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
			_gcWalkDfs(context, k);
		for (auto &k : j->minorFrames) {
			for (auto &l : k.exceptHandlers)
				_gcWalkDfs(context, l.type);
		}
	}
	return true;
}

SLAKE_API bool Runtime::_gcDfs() {
	bool foundDestructibleObjects = false;

rescan:
	GCDfsWalkContext context;
	for (auto i : createdObjects) {
		i->_flags |= VF_GCREADY;
		i->gcInfo.dfs.gcStatus = ObjectGCStatus::Unwalked;
		if (i->hostRefCount) {
			_gcWalkDfs(context, i);
		}
	}

	// Walk the root node.
	_gcWalkDfs(context, _rootObject);

	// Walk contexts for each thread.
	for (auto &i : activeContexts)
		_gcWalkDfs(context, i.second);

	for(;;) {
		std::set<Object *> walkableObjectSet = std::move(context.objectStack);
		context.objectStack = {};

		if (walkableObjectSet.empty())
			break;

		for (auto i : walkableObjectSet)
			_gcWalkDfs(context, i);
	};

	// Execute destructors for all destructible unreachable objects.
	destructingThreads.insert(std::this_thread::get_id());
	for (auto i : createdObjects) {
		switch (i->gcInfo.dfs.gcStatus) {
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
		switch (i->gcInfo.dfs.gcStatus) {
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
				i->gcInfo.dfs.gcStatus = ObjectGCStatus::Unwalked;
				break;
		}

		++it;
	}

	if (foundDestructibleObjects) {
		foundDestructibleObjects = false;
		goto rescan;
	}

	return true;
}

