#include "../compiler.h"

using namespace slake::slkc;

void Compiler::importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, BasicFnValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	string fnName = value->_name;
	size_t j = 0;

	while (j < value->_name.size())
		if (value->_name[j] == '$') {
			break;
		} else
			++j;

	fnName = fnName.substr(0, j);

	auto returnType = toTypeName(value->getReturnType());
	GenericParamNodeList genericParams;

	deque<Param> params;

	for (auto i : value->getParamTypes()) {
		Param param(Location(), toTypeName(i), "");

		params.push_back(param);
	}

	shared_ptr<FnOverloadingNode> overloading = make_shared<FnOverloadingNode>(Location(), this, returnType, genericParams, params);

	if (!scope->members.count(fnName))
		(scope->members[fnName] = make_shared<FnNode>(this, fnName))->bind(parent.get());

	static_pointer_cast<FnNode>(scope->members[fnName])->overloadingRegistries.push_back(overloading);
}

void Compiler::importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, ModuleValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	shared_ptr<ModuleNode> mod = make_shared<ModuleNode>(this, Location());

	(scope->members[value->_name] = mod)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(mod->scope, mod, i.second);
}

void Compiler::importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, ClassValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	MemberValue *parentClassValue = (MemberValue *)value->parentClass.resolveCustomType();
	if (!parentClassValue)
		assert(false);

	shared_ptr<CustomTypeNameNode> parentClassTypeName =
		make_shared<CustomTypeNameNode>(
			Location(),
			toAstRef(_rt->getFullRef(parentClassValue)),
			this,
			scope.get());

	deque<shared_ptr<TypeNameNode>> implInterfaceTypeNames;
	for (auto i : value->implInterfaces) {
		MemberValue *implInterfaceValue = (MemberValue *)(i.resolveCustomType());
		if (!implInterfaceValue)
			assert(false);

		auto implInterfaceRef = _rt->getFullRef(implInterfaceValue);

		implInterfaceTypeNames.push_back(
			make_shared<CustomTypeNameNode>(
				Location(),
				toAstRef(implInterfaceRef),
				this,
				scope.get()));
	}

	GenericParamNodeList genericParams;
	for (auto &i : value->genericParams) {
	}

	shared_ptr<ClassNode> cls = make_shared<ClassNode>(
		Location(),
		this,
		value->getName(),
		parentClassTypeName, implInterfaceTypeNames,
		genericParams);

	(scope->members[value->_name] = cls)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(cls->scope, cls, (Value *)i.second);
}

void Compiler::importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, InterfaceValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	shared_ptr<InterfaceNode> interface = make_shared<InterfaceNode>(
		Location(),
		value->_name,
		deque<shared_ptr<TypeNameNode>>{},
		GenericParamNodeList{});

	for (auto i : value->parents) {
		interface->parentInterfaces.push_back(toTypeName(i));
	}

	(scope->members[value->_name] = interface)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(scope, interface, (Value *)i.second);
}

void Compiler::importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, TraitValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	deque<shared_ptr<TypeNameNode>> parentTraits;

	for (auto i : value->parents) {
		parentTraits.push_back(toTypeName(i));
	}

	shared_ptr<TraitNode> trait = make_shared<TraitNode>(
		Location(),
		parentTraits,
		GenericParamNodeList()	// stub
	);

	(scope->members[value->_name] = trait)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(scope, trait, (Value *)i.second);
}

void Compiler::importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, Value *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	switch (value->getType().typeId) {
		case TypeId::RootValue: {
			RootValue *v = (RootValue *)value;

			for (auto i : v->scope->members)
				importDefinitions(scope, parent, i.second);

			break;
		}
		case TypeId::Fn:
			importDefinitions(scope, parent, (FnValue *)value);
			break;
		case TypeId::Module:
			importDefinitions(scope, parent, (ModuleValue *)value);
			break;
		case TypeId::Var: {
			VarValue *v = (VarValue *)value;
			shared_ptr<VarNode> var = make_shared<VarNode>(Location(), this, v->getAccess(), toTypeName(v->getVarType()), v->_name, shared_ptr<ExprNode>());

			scope->members[v->_name] = var;
			var->bind(parent.get());
			break;
		}
		case TypeId::Class:
			importDefinitions(scope, parent, (ClassValue *)value);
			break;
		case TypeId::Interface:
			importDefinitions(scope, parent, (InterfaceValue *)value);
			break;
		case TypeId::Trait:
			importDefinitions(scope, parent, (TraitValue *)value);
			break;
			/*
		case TypeId::Alias: {
			AliasValue *v = (AliasValue *)value;
		}*/
		default:
			// Ignored.
			;
	}
}

shared_ptr<TypeNameNode> Compiler::toTypeName(slake::Type runtimeType) {
	bool isConst = runtimeType.flags & TYPE_CONST;

	switch (runtimeType.typeId) {
		case TypeId::I8:
			return make_shared<I8TypeNameNode>(Location{}, isConst);
		case TypeId::I16:
			return make_shared<I16TypeNameNode>(Location{}, isConst);
		case TypeId::I32:
			return make_shared<I32TypeNameNode>(Location{}, isConst);
		case TypeId::I64:
			return make_shared<I64TypeNameNode>(Location{}, isConst);
		case TypeId::U8:
			return make_shared<U8TypeNameNode>(Location{}, isConst);
		case TypeId::U16:
			return make_shared<U16TypeNameNode>(Location{}, isConst);
		case TypeId::U32:
			return make_shared<U32TypeNameNode>(Location{}, isConst);
		case TypeId::U64:
			return make_shared<U64TypeNameNode>(Location{}, isConst);
		case TypeId::F32:
			return make_shared<F32TypeNameNode>(Location{}, isConst);
		case TypeId::F64:
			return make_shared<F64TypeNameNode>(Location{}, isConst);
		case TypeId::String:
			return make_shared<StringTypeNameNode>(Location{}, isConst);
		case TypeId::Bool:
			return make_shared<BoolTypeNameNode>(Location{}, isConst);
		case TypeId::None:
			return make_shared<VoidTypeNameNode>(Location{}, isConst);
		case TypeId::Any:
			return make_shared<AnyTypeNameNode>(Location{}, isConst);
		case TypeId::TypeName: {
			auto refs = _rt->getFullRef((MemberValue *)runtimeType.getCustomTypeExData());
			Ref ref;

			for (auto &i : refs) {
				deque<shared_ptr<TypeNameNode>> genericArgs;
				for (auto j : i.genericArgs) {
					genericArgs.push_back(toTypeName(j));
				}

				ref.push_back(RefEntry(Location{}, SIZE_MAX, i.name, genericArgs));
			}

			return make_shared<CustomTypeNameNode>(Location{}, ref, this, nullptr, isConst);
		}
		case TypeId::Array:
			return make_shared<ArrayTypeNameNode>(toTypeName(runtimeType.getArrayExData()), isConst);
		case TypeId::Map: {
			auto exData = runtimeType.getMapExData();
			return make_shared<MapTypeNameNode>(toTypeName(*exData.first), toTypeName(*exData.second), isConst);
		}
		default:
			// Inconvertible/unrecognized type
			assert(false);
	}
}

slake::slkc::Ref Compiler::toAstRef(std::deque<slake::RefEntry> runtimeRefEntries) {
	Ref ref;

	for (auto &i : runtimeRefEntries) {
		deque<shared_ptr<TypeNameNode>> genericArgs;

		for (auto j : i.genericArgs)
			genericArgs.push_back(toTypeName(j));

		ref.push_back(RefEntry(Location(), SIZE_MAX, i.name, genericArgs));
	}

	return ref;
}
