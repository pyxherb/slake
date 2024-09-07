#include "../runtime.h"

using namespace slake;

void Runtime::_gcWalk(Scope *scope) {
	if (!scope)
		return;
	for (auto &i : scope->members) {
		_gcWalk(i.second);
	}

	if (scope->owner)
		_gcWalk(scope->owner);
}

void Runtime::_gcWalk(MethodTable *methodTable) {
	if (!methodTable)
		return;
	for (auto &i : methodTable->methods) {
		_gcWalk(i.second);
	}
}

void Runtime::_gcWalk(GenericParamList &genericParamList) {
	for (auto &i : genericParamList) {
		i.baseType.loadDeferredType(this);
		if (auto p = i.baseType.resolveCustomType(); p)
			_gcWalk(p);

		for (auto &j : i.interfaces) {
			j.loadDeferredType(this);
			if (auto p = j.resolveCustomType(); p)
				_gcWalk(p);
		}
	}
}

void Runtime::_gcWalk(const Type &type) {
	switch (type.typeId) {
		case TypeId::Value:
		case TypeId::String:
			break;
		case TypeId::Instance:
		case TypeId::GenericArg:
			_gcWalk(type.getCustomTypeExData());
			break;
		case TypeId::Array:
			if (type.exData.ptr)
				_gcWalk(type.exData.ptr);
			break;
		case TypeId::Ref:
			if (type.exData.ptr)
				_gcWalk(type.exData.ptr);
			break;
		case TypeId::None:
		case TypeId::Any:
			break;
		default:
			throw std::logic_error("Unhandled object type");
	}
}

void Runtime::_gcWalk(const Value &i) {
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
			_gcWalk(i.getObjectRef().objectPtr);
			break;
		case ValueType::RegRef:
			break;
		case ValueType::TypeName:
			_gcWalk(i.getTypeName());
			break;
		case ValueType::VarRef: {
			auto &varRef = i.getVarRef();

			_gcWalk(varRef.varPtr);
			break;
		}
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

void Runtime::_gcWalk(Object *v) {
	if (!v)
		return;

	if (v->_flags & VF_WALKED)
		return;

	v->_flags |= VF_WALKED;

	switch (auto typeId = v->getKind(); typeId) {
		case ObjectKind::String:
			break;
		case ObjectKind::TypeDef:
			_gcWalk(((TypeDefObject *)v)->type);
			break;
		case ObjectKind::Instance: {
			auto value = (InstanceObject *)v;

			_gcWalk(value->_class);
			_gcWalk(value->methodTable);

			for (auto &i : value->objectLayout->fieldRecords) {
				_gcWalk(i.type);

				switch (i.type.typeId) {
					case TypeId::Value: {
						switch (i.type.getValueTypeExData()) {
							case ValueType::ObjectRef:
								_gcWalk(*((Object **)(value->rawFieldData + i.offset)));
								break;
						}
						break;
					}
					case TypeId::String:
					case TypeId::Instance:
					case TypeId::Array:
					case TypeId::Ref:
						_gcWalk(*((Object **)(value->rawFieldData + i.offset)));
						break;
				}
			}

			_gcWalk(value->_class);
			break;
		}
		case ObjectKind::Array: {
			auto value = (ArrayObject *)v;

			_gcWalk(value->elementType);
			_gcWalk(value->accessor);

			switch (value->elementType.typeId) {
				case TypeId::Instance: {
					auto v = (ObjectRefArrayObject *)value;
					for (size_t i = 0; i < v->length; ++i)
						_gcWalk(v->data[i]);
					break;
				}
			}
			break;
		}
		case ObjectKind::Module:
		case ObjectKind::Class:
		case ObjectKind::Interface: {
			// TODO: Walk generic parameters.

			_gcWalk(((ModuleObject *)v)->scope);
			_gcWalk(((ModuleObject *)v)->parent);

			for (auto &i : ((ModuleObject *)v)->imports)
				_gcWalk(i.second);

			switch (typeId) {
				case ObjectKind::Class: {
					ClassObject *value = (ClassObject *)v;
					for (auto &i : value->implInterfaces) {
						i.loadDeferredType(this);
						_gcWalk(i);
					}
					for (auto &i : value->genericParams) {
						i.baseType.loadDeferredType(this);
						_gcWalk(i.baseType);
						for (auto &j : i.interfaces) {
							j.loadDeferredType(this);
							_gcWalk(j);
						}
					}
					for (auto &i : value->genericArgs) {
						i.loadDeferredType(this);
						_gcWalk(i);
					}

					value->parentClass.loadDeferredType(this);
					_gcWalk(value->parentClass.resolveCustomType());

					_gcWalk(value->genericParams);
					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *value = (InterfaceObject *)v;

					for (auto &i : value->parents) {
						i.loadDeferredType(this);
						_gcWalk(i.getCustomTypeExData());
					}
					for (auto &i : value->genericParams) {
						i.baseType.loadDeferredType(this);
						_gcWalk(i.baseType);
						for (auto &j : i.interfaces) {
							j.loadDeferredType(this);
							_gcWalk(j);
						}
					}
					for (auto &i : value->genericArgs) {
						i.loadDeferredType(this);
						_gcWalk(i);
					}

					_gcWalk(value->genericParams);
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

					_gcWalk(value->getData(VarRefContext()));

					_gcWalk(v->parent);

					_gcWalk(v->type);
					break;
				}
				case VarKind::ArrayElementAccessor: {
					auto v = (ArrayAccessorVarObject *)value;

					_gcWalk(v->arrayObject);
					break;
				}
				case VarKind::InstanceMemberAccessor: {
					auto v = (InstanceMemberAccessorVarObject *)value;

					_gcWalk(v->instanceObject);
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
			_gcWalk(value->scope);
			break;
		}
		case ObjectKind::Fn: {
			auto fn = (FnObject *)v;

			_gcWalk(fn->getParent());

			for (auto &i : fn->overloadings) {
				_gcWalk(i);
			}
			break;
		}
		case ObjectKind::FnOverloading: {
			auto fnOverloading = (FnOverloadingObject *)v;

			_gcWalk(fnOverloading->fnObject);

			for (auto &i : fnOverloading->genericParams) {
				i.baseType.loadDeferredType(this);
				_gcWalk(i.baseType);
				for (auto &j : i.interfaces) {
					j.loadDeferredType(this);
					_gcWalk(j);
				}
			}
			for (auto &i : fnOverloading->mappedGenericArgs) {
				i.second.loadDeferredType(this);
				_gcWalk(i.second);
			}

			for (auto &j : fnOverloading->paramTypes)
				_gcWalk(j);
			_gcWalk(fnOverloading->returnType);

			switch (fnOverloading->getOverloadingKind()) {
				case FnOverloadingKind::Regular: {
					RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fnOverloading;

					for (auto &i : ol->instructions) {
						for (auto &j : i.operands) {
							_gcWalk(j);
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

			for (auto &i : value->entries)
				for (auto &j : i.genericArgs) {
					_gcWalk(j);
				}
			break;
		}
		case ObjectKind::Alias: {
			auto value = (AliasObject *)v;

			_gcWalk(value->src);
			break;
		}
		case ObjectKind::Context: {
			auto value = (ContextObject *)v;

			_gcWalk(*value->_context);
			break;
		}
		default:
			throw std::logic_error("Unhandled object type");
	}
}

void Runtime::_gcWalk(Context &ctxt) {
	for (auto &j : ctxt.majorFrames) {
		_gcWalk((FnOverloadingObject *)j->curFn);
		_gcWalk(j->scopeObject);
		_gcWalk(j->returnValue);
		_gcWalk(j->thisObject);
		_gcWalk(j->curExcept);
		for (auto &k : j->argStack)
			_gcWalk((VarObject *)k);
		for (auto &k : j->nextArgStack)
			_gcWalk(k);
		for (auto &k : j->localVarRecords) {
			_gcWalk(k.type);

			switch (k.type.typeId) {
				case TypeId::Value: {
					switch (k.type.getValueTypeExData()) {
						case ValueType::ObjectRef:
							_gcWalk(*((Object **)(ctxt.dataStack + k.stackOffset)));
							break;
					}
					break;
				}
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Ref:
					_gcWalk(*((Object **)(ctxt.dataStack + k.stackOffset)));
					break;
			}
		}
		_gcWalk(j->localVarAccessor);
		for (auto &k : j->regs)
			_gcWalk(k);
		for (auto &k : j->minorFrames) {
			for (auto &l : k.exceptHandlers)
				_gcWalk(l.type);
		}
	}
}

void Runtime::gc() {
	_flags |= _RT_INGC;

	bool foundDestructibleObjects = false;

rescan:
	for (auto i : createdObjects) {
		i->_flags |= VF_GCREADY;
	}

	// Walk the root node.
	_gcWalk(_rootObject);

	// Walk contexts for each thread.
	for (auto &i : activeContexts)
		_gcWalk(*i.second);

	// Walk all objects referenced by the host.
	for (auto i : createdObjects) {
		if (i->hostRefCount) {
			_gcWalk(i);
		}
	}

	// Execute destructors for all destructible unreachable objects.
	destructingThreads.insert(std::this_thread::get_id());
	for (auto i : createdObjects) {
		if (i->_flags & VF_WALKED)
			continue;

		if (i->getKind() == ObjectKind::Instance) {
			InstanceObject *object = (InstanceObject *)i;

			if (auto mt = object->methodTable; mt) {
				if (mt->destructors.size()) {
					for (auto i : mt->destructors) {
						HostRefHolder holder;
						i->call(i, {}, &holder);
					}
					foundDestructibleObjects = true;
				}
			}
		}
	}
	destructingThreads.erase(std::this_thread::get_id());

	// Delete unreachable objects.
	for (auto it = createdObjects.begin(); it != createdObjects.end();) {
		if ((*it)->_flags & VF_WALKED) {
			(*it)->_flags &= ~VF_WALKED;
		} else {
			if ((*it)->_flags & VF_GCREADY) {
				(*it)->dealloc();
				createdObjects.erase(*(it++));
				continue;
			}
		}

		++it;
	}

	if (foundDestructibleObjects) {
		foundDestructibleObjects = false;
		goto rescan;
	}

	_szMemUsedAfterLastGc = globalHeapPoolResource.szAllocated;
	_flags &= ~_RT_INGC;
}
