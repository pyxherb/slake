#include "../compiler.h"
#include <slake/util/stream.hh>
#include <slake/util/scope_guard.h>

using namespace slake::slkc;

void Compiler::importModule(std::shared_ptr<IdRefNode> ref) {
	if (importedModules.count(ref->entries))
		return;
	ModuleImportInfo &importInfo = importedModules[ref->entries];
	slake::util::ScopeGuard importInfoRemovalGuard([this, &importInfo, &ref]() {
		importedModules.erase(ref->entries);
	});

	auto scope = completeModuleNamespaces(ref);

	std::string path;

	for (auto j : ref->entries) {
		path += "/" + j.name;
	}

	std::string savedCurDocName = std::move(curDocName);

	std::ifstream is;
	for (auto j : modulePaths) {
		is.open(j + path + ".slk");

		if (is.good()) {
			path = j + path + ".slk";

			util::PseudoOutputStream pseudoOs;

			curDocName = path;
			addDoc(curDocName);

			compile(is, pseudoOs);

			goto succeeded;
		}

		is.clear();

		is.open(j + path + ".slx");

		if (is.good()) {
			path = j + path + ".slx";

			try {
				auto mod = associatedRuntime->loadModule(is, LMOD_NOIMPORT | LMOD_NORELOAD);

				for (auto j : mod->imports)
					importModule(toAstIdRef(j.second->entries));

				for (auto j : mod->unnamedImports)
					importModule(toAstIdRef(j->entries));

				importDefinitions(scope, {}, mod.get());

				goto succeeded;
			} catch (LoaderError e) {
				printf("%s\n", e.what());
			}
		}

		is.clear();
	}

	curDocName = std::move(savedCurDocName);

	throw FatalCompilationError(
		Message(
			tokenRangeToSourceLocation(ref->entries[0].tokenRange),
			MessageType::Error,
			"Cannot find module " + std::to_string(ref->entries, this)));

succeeded:;
	importInfoRemovalGuard.release();
	curDocName = std::move(savedCurDocName);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, FnObject *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	std::string fnName = value->getName();

	for (auto &i : value->overloadings) {
		auto returnType = toTypeName(i->returnType);
		GenericParamNodeList genericParams;

		std::deque<std::shared_ptr<ParamNode>> params;

		for (auto i : i->paramTypes) {
			std::shared_ptr<ParamNode> param = std::make_shared<ParamNode>();
			param->type = toTypeName(i);

			params.push_back(param);
		}

		std::shared_ptr<FnOverloadingNode> overloading = std::make_shared<FnOverloadingNode>(this, std::make_shared<Scope>());
		overloading->returnType = returnType;
		overloading->setGenericParams(genericParams);
		overloading->params = params;

		if (i->overloadingFlags & OL_VARG) {
			auto param = std::make_shared<ParamNode>();
			param->type = std::make_shared<ArrayTypeNameNode>(std::make_shared<AnyTypeNameNode>(SIZE_MAX));
			param->name = "...";
			overloading->varArgParam = param;
		}

		overloading->updateParamIndices();

		overloading->isImported = true;

		if (!scope->members.count(fnName))
			(scope->members[fnName] = std::make_shared<FnNode>(this, fnName))->bind(parent.get());

		scope->members[fnName]->isImported = true;

		std::static_pointer_cast<FnNode>(scope->members[fnName])->overloadingRegistries.push_back(overloading);
	}
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ModuleObject *value) {
	if (importedDefinitions.count(value))
		return;

	auto fullRef = toAstIdRef(associatedRuntime->getFullRef(value));
	auto s = completeModuleNamespaces(fullRef);
	std::shared_ptr<MemberNode> owner = std::static_pointer_cast<MemberNode>(scope->owner->shared_from_this());

	for (auto i : value->scope->members)
		importDefinitions(s, owner, i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ClassObject *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	MemberObject *parentClassObject = (MemberObject *)value->parentClass.resolveCustomType();
	if (!parentClassObject)
		assert(false);

	std::shared_ptr<CustomTypeNameNode> parentClassTypeName =
		std::make_shared<CustomTypeNameNode>(
			toAstIdRef(associatedRuntime->getFullRef(parentClassObject)),
			this,
			scope.get());

	std::deque<std::shared_ptr<TypeNameNode>> implInterfaceTypeNames;
	for (auto i : value->implInterfaces) {
		MemberObject *implInterfaceObject = (MemberObject *)(i.resolveCustomType());
		if (!implInterfaceObject)
			assert(false);

		auto implInterfaceRef = associatedRuntime->getFullRef(implInterfaceObject);

		implInterfaceTypeNames.push_back(
			std::make_shared<CustomTypeNameNode>(
				toAstIdRef(implInterfaceRef),
				this,
				scope.get()));
	}

	GenericParamNodeList genericParams;
	for (auto &i : value->genericParams) {
	}

	std::shared_ptr<ClassNode> cls = std::make_shared<ClassNode>(
		this,
		value->getName());

	cls->parentClass = parentClassTypeName;
	cls->implInterfaces = implInterfaceTypeNames;
	cls->genericParams = genericParams;

	(scope->members[value->getName()] = cls)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(cls->scope, cls, (Object *)i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, InterfaceObject *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	std::shared_ptr<InterfaceNode> interface = std::make_shared<InterfaceNode>(value->getName());

	for (auto i : value->parents) {
		interface->parentInterfaces.push_back(toTypeName(i));
	}

	(scope->members[value->getName()] = interface)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(scope, interface, (Object *)i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, Object *value) {
	if (importedDefinitions.count(value))
		return;

	switch (value->getKind()) {
		case slake::ObjectKind::RootObject: {
			RootObject *v = (RootObject *)value;

			for (auto i : v->scope->members)
				importDefinitions(scope, parent, i.second);

			break;
		}
		case slake::ObjectKind::Fn:
			importDefinitions(scope, parent, (FnObject *)value);
			break;
		case slake::ObjectKind::Module:
			importDefinitions(scope, parent, (ModuleObject *)value);
			break;
		case slake::ObjectKind::Var: {
			RegularVarObject *v = (RegularVarObject *)value;
			std::shared_ptr<VarNode> var = std::make_shared<VarNode>(
				this,
				v->accessModifier,
				toTypeName(v->getVarType(VarRefContext())),
				v->getName(),
				std::shared_ptr<ExprNode>(),
				SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX);

			scope->members[v->getName()] = var;
			var->bind(parent.get());
			break;
		}
		case slake::ObjectKind::Class:
			importDefinitions(scope, parent, (ClassObject *)value);
			break;
		case slake::ObjectKind::Interface:
			importDefinitions(scope, parent, (InterfaceObject *)value);
			break;
			/*
		case slake::ObjectKind::Alias: {
			AliasValue *v = (AliasValue *)value;
		}*/
		default:
			// Ignored.
			;
	}
}

std::shared_ptr<TypeNameNode> Compiler::toTypeName(slake::Type runtimeType) {
	switch (runtimeType.typeId) {
		case slake::TypeId::Value:
			switch (runtimeType.getValueTypeExData()) {
				case slake::ValueType::I8:
					return std::make_shared<I8TypeNameNode>(SIZE_MAX);
				case slake::ValueType::I16:
					return std::make_shared<I16TypeNameNode>(SIZE_MAX);
				case slake::ValueType::I32:
					return std::make_shared<I32TypeNameNode>(SIZE_MAX);
				case slake::ValueType::I64:
					return std::make_shared<I64TypeNameNode>(SIZE_MAX);
				case slake::ValueType::U8:
					return std::make_shared<U8TypeNameNode>(SIZE_MAX);
				case slake::ValueType::U16:
					return std::make_shared<U16TypeNameNode>(SIZE_MAX);
				case slake::ValueType::U32:
					return std::make_shared<U32TypeNameNode>(SIZE_MAX);
				case slake::ValueType::U64:
					return std::make_shared<U64TypeNameNode>(SIZE_MAX);
				case slake::ValueType::F32:
					return std::make_shared<F32TypeNameNode>(SIZE_MAX);
				case slake::ValueType::F64:
					return std::make_shared<F64TypeNameNode>(SIZE_MAX);
				case slake::ValueType::Bool:
					return std::make_shared<BoolTypeNameNode>(SIZE_MAX);
				case slake::ValueType::TypeName: {
					auto refs = associatedRuntime->getFullRef((MemberObject *)runtimeType.getCustomTypeExData());
					std::shared_ptr<IdRefNode> ref = std::make_shared<IdRefNode>();

					for (auto &i : refs) {
						std::deque<std::shared_ptr<TypeNameNode>> genericArgs;
						for (auto j : i.genericArgs) {
							genericArgs.push_back(toTypeName(j));
						}

						ref->entries.push_back(IdRefEntry({}, SIZE_MAX, std::string(i.name.c_str(), i.name.size()), genericArgs));
					}

					return std::make_shared<CustomTypeNameNode>(ref, this, nullptr);
				}
			}
		case slake::TypeId::String:
			return std::make_shared<StringTypeNameNode>(SIZE_MAX);
		case slake::TypeId::None:
			return std::make_shared<VoidTypeNameNode>(SIZE_MAX);
		case slake::TypeId::Any:
			return std::make_shared<AnyTypeNameNode>(SIZE_MAX);
		case slake::TypeId::Array:
			return std::make_shared<ArrayTypeNameNode>(toTypeName(runtimeType.getArrayExData()));
		default:
			break;
	}
	throw std::logic_error("Unrecognized runtime value type");
}

std::shared_ptr<IdRefNode> Compiler::toAstIdRef(std::pmr::deque<slake::IdRefEntry> runtimeRefEntries) {
	std::shared_ptr<IdRefNode> ref = std::make_shared<IdRefNode>();

	for (auto &i : runtimeRefEntries) {
		std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

		for (auto j : i.genericArgs)
			genericArgs.push_back(toTypeName(j));

		ref->entries.push_back(IdRefEntry({}, SIZE_MAX, std::string(i.name.c_str(), i.name.size()), genericArgs));
	}

	return ref;
}

std::shared_ptr<IdRefNode> Compiler::toAstIdRef(std::deque<slake::IdRefEntry> runtimeRefEntries) {
	std::shared_ptr<IdRefNode> ref = std::make_shared<IdRefNode>();

	for (auto &i : runtimeRefEntries) {
		std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

		for (auto j : i.genericArgs)
			genericArgs.push_back(toTypeName(j));

		ref->entries.push_back(IdRefEntry({}, SIZE_MAX, std::string(i.name.c_str(), i.name.size()), genericArgs));
	}

	return ref;
}
