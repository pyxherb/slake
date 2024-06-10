#include "../compiler.h"
#include <slake/util/stream.hh>

using namespace slake::slkc;

void Compiler::importModule(const IdRef &ref) {
	if (importedModules.count(ref))
		return;
	importedModules.insert(ref);

	auto scope = completeModuleNamespaces(ref);

	std::string path;

	for (auto j : ref) {
		path += "/" + j.name;
	}

	auto savedLexer = std::move(lexer);
#if SLKC_WITH_LANGUAGE_SERVER
	auto savedTokenInfos = tokenInfos;
#endif

	std::ifstream is;
	for (auto j : modulePaths) {
		is.open(j + path + ".slk");

		if (is.good()) {
			path = j + path + ".slk";

			auto savedTargetModule = _targetModule;
			util::PseudoOutputStream pseudoOs;
			compile(is, pseudoOs);
			_targetModule = savedTargetModule;

			goto succeeded;
		}

		is.clear();

		is.open(j + path + ".slx");

		if (is.good()) {
			path = j + path + ".slx";

			try {
				auto mod = _rt->loadModule(is, LMOD_NOIMPORT | LMOD_NORELOAD);

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

#if SLKC_WITH_LANGUAGE_SERVER
	tokenInfos = savedTokenInfos;
#endif
	lexer = std::move(savedLexer);

	throw FatalCompilationError(
		Message(
			ref[0].loc,
			MessageType::Error,
			"Cannot find module " + std::to_string(ref, this)));

succeeded:;
#if SLKC_WITH_LANGUAGE_SERVER
	tokenInfos = savedTokenInfos;
#endif
	lexer = std::move(savedLexer);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, FnObject *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	std::string fnName = value->_name;
	size_t j = 0;

	fnName = fnName.substr(0, j);

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
			overloading->params.push_back(param);
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

	auto fullRef = toAstIdRef(_rt->getFullRef(value));
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
			toAstIdRef(_rt->getFullRef(parentClassObject)),
			this,
			scope.get());

	std::deque<std::shared_ptr<TypeNameNode>> implInterfaceTypeNames;
	for (auto i : value->implInterfaces) {
		MemberObject *implInterfaceObject = (MemberObject *)(i.resolveCustomType());
		if (!implInterfaceObject)
			assert(false);

		auto implInterfaceRef = _rt->getFullRef(implInterfaceObject);

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

	(scope->members[value->_name] = cls)->bind(parent.get());

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

	(scope->members[value->_name] = interface)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(scope, interface, (Object *)i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, Object *value) {
	if (importedDefinitions.count(value))
		return;

	switch (value->getType().typeId) {
		case slake::TypeId::RootObject: {
			RootObject *v = (RootObject *)value;

			for (auto i : v->scope->members)
				importDefinitions(scope, parent, i.second);

			break;
		}
		case slake::TypeId::Fn:
			importDefinitions(scope, parent, (FnObject *)value);
			break;
		case slake::TypeId::Module:
			importDefinitions(scope, parent, (ModuleObject *)value);
			break;
		case slake::TypeId::Var: {
			VarObject *v = (VarObject *)value;
			std::shared_ptr<VarNode> var = std::make_shared<VarNode>(
				this,
				v->getAccess(),
				toTypeName(v->getVarType()),
				v->_name,
				std::shared_ptr<ExprNode>(),
				SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX);

			scope->members[v->_name] = var;
			var->bind(parent.get());
			break;
		}
		case slake::TypeId::Class:
			importDefinitions(scope, parent, (ClassObject *)value);
			break;
		case slake::TypeId::Interface:
			importDefinitions(scope, parent, (InterfaceObject *)value);
			break;
			/*
		case slake::TypeId::Alias: {
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
					auto refs = _rt->getFullRef((MemberObject *)runtimeType.getCustomTypeExData());
					IdRef ref;

					for (auto &i : refs) {
						std::deque<std::shared_ptr<TypeNameNode>> genericArgs;
						for (auto j : i.genericArgs) {
							genericArgs.push_back(toTypeName(j));
						}

						ref.push_back(IdRefEntry(SourceLocation{}, SIZE_MAX, i.name, genericArgs));
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

slake::slkc::IdRef Compiler::toAstIdRef(std::deque<slake::IdRefEntry> runtimeRefEntries) {
	IdRef ref;

	for (auto &i : runtimeRefEntries) {
		std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

		for (auto j : i.genericArgs)
			genericArgs.push_back(toTypeName(j));

		ref.push_back(IdRefEntry(SourceLocation{}, SIZE_MAX, i.name, genericArgs));
	}

	return ref;
}
