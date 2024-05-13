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
			compile(is, pseudoOs, true);
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

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, BasicFnValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	std::string fnName = value->_name;
	size_t j = 0;

	while (j < value->_name.size())
		if (value->_name[j] == '$') {
			break;
		} else
			++j;

	fnName = fnName.substr(0, j);

	auto returnType = toTypeName(value->getReturnType());
	GenericParamNodeList genericParams;

	std::deque<std::shared_ptr<ParamNode>> params;

	for (auto i : value->getParamTypes()) {
		std::shared_ptr<ParamNode> param = std::make_shared<ParamNode>(Location(), toTypeName(i));

		params.push_back(param);
	}

	std::shared_ptr<FnOverloadingNode> overloading = std::make_shared<FnOverloadingNode>(Location(), this, std::make_shared<Scope>());
	overloading->returnType = returnType;
	overloading->setGenericParams(genericParams);
	overloading->params = params;

	if (value->fnFlags & FN_VARG) {
		auto param = std::make_shared<ParamNode>(Location(), std::make_shared<ArrayTypeNameNode>(std::make_shared<AnyTypeNameNode>(Location(), SIZE_MAX)));
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

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ModuleValue *value) {
	if (importedDefinitions.count(value))
		return;

	auto fullRef = toAstIdRef(_rt->getFullRef(value));
	auto s = completeModuleNamespaces(fullRef);
	std::shared_ptr<MemberNode> owner = std::static_pointer_cast<MemberNode>(scope->owner->shared_from_this());

	for (auto i : value->scope->members)
		importDefinitions(s, owner, i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ClassValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	MemberValue *parentClassValue = (MemberValue *)value->parentClass.resolveCustomType();
	if (!parentClassValue)
		assert(false);

	std::shared_ptr<CustomTypeNameNode> parentClassTypeName =
		std::make_shared<CustomTypeNameNode>(
			Location(),
			toAstIdRef(_rt->getFullRef(parentClassValue)),
			this,
			scope.get());

	std::deque<std::shared_ptr<TypeNameNode>> implInterfaceTypeNames;
	for (auto i : value->implInterfaces) {
		MemberValue *implInterfaceValue = (MemberValue *)(i.resolveCustomType());
		if (!implInterfaceValue)
			assert(false);

		auto implInterfaceRef = _rt->getFullRef(implInterfaceValue);

		implInterfaceTypeNames.push_back(
			std::make_shared<CustomTypeNameNode>(
				Location(),
				toAstIdRef(implInterfaceRef),
				this,
				scope.get()));
	}

	GenericParamNodeList genericParams;
	for (auto &i : value->genericParams) {
	}

	std::shared_ptr<ClassNode> cls = std::make_shared<ClassNode>(
		Location(),
		this,
		value->getName());

	cls->parentClass = parentClassTypeName;
	cls->implInterfaces = implInterfaceTypeNames;
	cls->genericParams = genericParams;

	(scope->members[value->_name] = cls)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(cls->scope, cls, (Value *)i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, InterfaceValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	std::shared_ptr<InterfaceNode> interface = std::make_shared<InterfaceNode>(
		Location(),
		value->getName());

	for (auto i : value->parents) {
		interface->parentInterfaces.push_back(toTypeName(i));
	}

	(scope->members[value->_name] = interface)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(scope, interface, (Value *)i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, TraitValue *value) {
	if (importedDefinitions.count(value))
		return;

	importedDefinitions.insert(value);

	std::deque<std::shared_ptr<TypeNameNode>> parentTraits;

	for (auto i : value->parents) {
		parentTraits.push_back(toTypeName(i));
	}

	std::shared_ptr<TraitNode> trait = std::make_shared<TraitNode>(
		Location(),
		value->getName());

	trait->parentTraits = parentTraits;

	(scope->members[value->_name] = trait)->bind(parent.get());

	for (auto i : value->scope->members)
		importDefinitions(scope, trait, (Value *)i.second);
}

void Compiler::importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, Value *value) {
	if (importedDefinitions.count(value))
		return;

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
			std::shared_ptr<VarNode> var = std::make_shared<VarNode>(
				Location(), this,
				v->getAccess(),
				toTypeName(v->getVarType()),
				v->_name,
				std::shared_ptr<ExprNode>(),
				SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX);

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

std::shared_ptr<TypeNameNode> Compiler::toTypeName(slake::Type runtimeType) {
	bool isConst = runtimeType.flags & TYPE_CONST;

	switch (runtimeType.typeId) {
		case TypeId::I8:
			return std::make_shared<I8TypeNameNode>(Location{}, isConst);
		case TypeId::I16:
			return std::make_shared<I16TypeNameNode>(Location{}, isConst);
		case TypeId::I32:
			return std::make_shared<I32TypeNameNode>(Location{}, isConst);
		case TypeId::I64:
			return std::make_shared<I64TypeNameNode>(Location{}, isConst);
		case TypeId::U8:
			return std::make_shared<U8TypeNameNode>(Location{}, isConst);
		case TypeId::U16:
			return std::make_shared<U16TypeNameNode>(Location{}, isConst);
		case TypeId::U32:
			return std::make_shared<U32TypeNameNode>(Location{}, isConst);
		case TypeId::U64:
			return std::make_shared<U64TypeNameNode>(Location{}, isConst);
		case TypeId::F32:
			return std::make_shared<F32TypeNameNode>(Location{}, isConst);
		case TypeId::F64:
			return std::make_shared<F64TypeNameNode>(Location{}, isConst);
		case TypeId::String:
			return std::make_shared<StringTypeNameNode>(Location{}, isConst);
		case TypeId::Bool:
			return std::make_shared<BoolTypeNameNode>(Location{}, isConst);
		case TypeId::None:
			return std::make_shared<VoidTypeNameNode>(Location{}, isConst);
		case TypeId::Any:
			return std::make_shared<AnyTypeNameNode>(Location{}, isConst);
		case TypeId::TypeName: {
			auto refs = _rt->getFullRef((MemberValue *)runtimeType.getCustomTypeExData());
			IdRef ref;

			for (auto &i : refs) {
				std::deque<std::shared_ptr<TypeNameNode>> genericArgs;
				for (auto j : i.genericArgs) {
					genericArgs.push_back(toTypeName(j));
				}

				ref.push_back(IdRefEntry(Location{}, SIZE_MAX, i.name, genericArgs));
			}

			return std::make_shared<CustomTypeNameNode>(Location{}, ref, this, nullptr, isConst);
		}
		case TypeId::Array:
			return std::make_shared<ArrayTypeNameNode>(toTypeName(runtimeType.getArrayExData()), isConst);
		default:
			// Inconvertible/unrecognized type
			throw std::logic_error("Unrecognized runtime value type");
	}
}

slake::slkc::IdRef Compiler::toAstIdRef(std::deque<slake::IdRefEntry> runtimeRefEntries) {
	IdRef ref;

	for (auto &i : runtimeRefEntries) {
		std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

		for (auto j : i.genericArgs)
			genericArgs.push_back(toTypeName(j));

		ref.push_back(IdRefEntry(Location(), SIZE_MAX, i.name, genericArgs));
	}

	return ref;
}
