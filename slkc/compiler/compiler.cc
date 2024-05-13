#include "compiler.h"
#include <slake/util/stream.hh>

using namespace slake::slkc;

template <typename T>
static void _write(std::ostream &fs, const T &value) {
	fs.write((const char *)&value, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, T &&value) {
	const T v = value;
	fs.write((const char *)&v, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, const T *ptr, size_t size) {
	fs.write((const char *)ptr, size);
}

Compiler::~Compiler() {
	flags |= COMP_DELETING;
}

std::shared_ptr<Scope> slake::slkc::Compiler::completeModuleNamespaces(const IdRef &ref) {
	auto scope = _rootScope;

	for (size_t i = 0; i < ref.size(); ++i) {
		std::string name = ref[i].name;

#if SLKC_WITH_LANGUAGE_SERVER
		if (ref[i].idxToken != SIZE_MAX) {
			auto &tokenInfo = tokenInfos[ref[i].idxToken];
			tokenInfo.semanticType = i == 0 ? SemanticType::Var : SemanticType::Property;
		}

		if (ref[i].idxAccessOpToken != SIZE_MAX) {
			auto &tokenInfo = tokenInfos[ref[i].idxAccessOpToken];
			tokenInfo.semanticType = i == 0 ? SemanticType::Var : SemanticType::Property;
		}
#endif

		if (auto it = scope->members.find(name); it != scope->members.end()) {
			switch (it->second->getNodeType()) {
				case NodeType::Class:
					scope = std::static_pointer_cast<ClassNode>(it->second)->scope;
					break;
				case NodeType::Interface:
					scope = std::static_pointer_cast<InterfaceNode>(it->second)->scope;
					break;
				case NodeType::Trait:
					scope = std::static_pointer_cast<TraitNode>(it->second)->scope;
					break;
				case NodeType::Module:
					scope = std::static_pointer_cast<ModuleNode>(it->second)->scope;
					break;
				default:
					throw FatalCompilationError(Message{
						ref[i].loc,
						MessageType::Error,
						"Cannot import a non-module member" });
			}
		} else {
			auto newMod = std::make_shared<ModuleNode>(this, Location());
			(scope->members[name] = newMod)->bind((MemberNode *)scope->owner);
			newMod->scope->parent = scope.get();
			newMod->moduleName = { IdRefEntry(Location(), SIZE_MAX, name, {}) };
			scope = newMod->scope;
		}
	}

	return scope;
}

void Compiler::compile(std::istream &is, std::ostream &os, bool isImport, std::shared_ptr<ModuleNode> targetModule) {
	if (targetModule)
		_targetModule = targetModule;
	else
		_targetModule = std::make_shared<ModuleNode>(this, Location());

	//
	// Clear the previous generic cache.
	// Note that we don't clear the generic cache after every compilation immediately,
	// because this will cause some generic instances referenced as parent value by member values
	// to be expired, this will influence services that analyzes the final compilation status,
	// such as language server.
	//
	_genericCacheDir.clear();

	lexer.reset();

	std::string s;
	{
		is.seekg(0, std::ios::end);
		size_t size = is.tellg();
		s.resize(size);
		is.seekg(0, std::ios::beg);
		is.read(s.data(), size);
	}

	lexer = std::make_unique<Lexer>();
	try {
		lexer->lex(s);
	} catch (LexicalError e) {
		throw FatalCompilationError(
			Message(
				e.location,
				MessageType::Error,
				"Lexical error"));
	}

#if SLKC_WITH_LANGUAGE_SERVER
	auto savedTokenInfos = tokenInfos;
	tokenInfos.resize(lexer->tokens.size());
#endif

	Parser parser;
	try {
		parser.parse(lexer.get(), this);
	} catch (SyntaxError e) {
		throw FatalCompilationError(
			Message(
				e.location,
				MessageType::Error,
				e.what()));
	}

#if SLKC_WITH_LANGUAGE_SERVER
	if (!isImport) {
		updateCompletionContext(_targetModule->moduleName, CompletionContext::ModuleName);
		updateSemanticType(_targetModule->moduleName, SemanticType::Var);

		IdRef curRef;
		for (size_t i = 0; i < _targetModule->moduleName.size(); ++i) {
			if (_targetModule->moduleName[i].idxToken != SIZE_MAX) {
				auto &tokenInfo = tokenInfos[_targetModule->moduleName[i].idxToken];
				tokenInfo.semanticInfo.importedPath = curRef;
			}

			if (_targetModule->moduleName[i].idxAccessOpToken != SIZE_MAX) {
				auto &tokenInfo = tokenInfos[_targetModule->moduleName[i].idxAccessOpToken];
				tokenInfo.semanticInfo.importedPath = curRef;
			}

			curRef.push_back(_targetModule->moduleName[i]);
		}
	}
#endif

	{
		slxfmt::ImgHeader ih = {};

		memcpy(ih.magic, slxfmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = (uint16_t)(_targetModule->imports.size() + _targetModule->unnamedImports.size());

		if (_targetModule->moduleName.size())
			ih.flags |= slxfmt::IMH_MODNAME;

		os.write((char *)&ih, sizeof(ih));
	}

	if (!isCompleteIdRef(_targetModule->moduleName)) {
		throw FatalCompilationError(Message(
			_targetModule->moduleName.back().loc,
			MessageType::Error,
			"Expecting a complete module name"));
	}

	if (_targetModule->moduleName.size()) {
		auto trimmedModuleName = _targetModule->moduleName;
		trimmedModuleName.pop_back();

		auto scope = completeModuleNamespaces(trimmedModuleName);

		(scope->members[_targetModule->moduleName.back().name] = _targetModule)->bind((MemberNode *)scope->owner);
		_targetModule->scope->parent = scope.get();

		compileIdRef(os, _targetModule->moduleName);
	}

	pushMajorContext();

#if SLKC_WITH_LANGUAGE_SERVER
	for (auto &i : _targetModule->imports) {
		if (i.second.idxNameToken != SIZE_MAX) {
			// Update corresponding semantic information.0
			auto &tokenInfo = tokenInfos[i.second.idxNameToken];
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Property;
		}

		updateCompletionContext(i.second.ref, CompletionContext::Import);
		updateSemanticType(i.second.ref, SemanticType::Property);

		IdRef importedPath;
		for (size_t j = 0; j < i.second.ref.size(); ++j) {
			if (i.second.ref[j].idxAccessOpToken != SIZE_MAX) {
				auto &tokenInfo = tokenInfos[i.second.ref[j].idxAccessOpToken];
				tokenInfo.semanticInfo.importedPath = importedPath;
			}

			if (i.second.ref[j].idxToken != SIZE_MAX) {
				auto &tokenInfo = tokenInfos[i.second.ref[j].idxToken];
				tokenInfo.semanticInfo.importedPath = importedPath;
			}

			importedPath.push_back(i.second.ref[j]);
		}
	}

	for (auto &i : _targetModule->unnamedImports) {
		updateCompletionContext(i.ref, CompletionContext::Import);
		updateSemanticType(i.ref, SemanticType::Property);

		IdRef importedPath;
		for (size_t j = 0; j < i.ref.size(); ++j) {
			if (i.ref[j].idxAccessOpToken != SIZE_MAX) {
				auto &tokenInfo = tokenInfos[i.ref[j].idxAccessOpToken];
				tokenInfo.semanticInfo.importedPath = importedPath;
			}

			if (i.ref[j].idxToken != SIZE_MAX) {
				auto &tokenInfo = tokenInfos[i.ref[j].idxToken];
				tokenInfo.semanticInfo.importedPath = importedPath;
			}

			importedPath.push_back(i.ref[j]);
		}
	}
#endif

	for (auto &i : _targetModule->imports) {
		// Skip bad references.
		// if (!isCompleteIdRef(i.second.ref))
		//	continue;

		_write(os, (uint32_t)i.first.size());
		_write(os, i.first.data(), i.first.length());
		compileIdRef(os, i.second.ref);

		importModule(i.second.ref);

		if (_targetModule->scope->members.count(i.first))
			throw FatalCompilationError(
				Message(
					lexer->tokens[i.second.idxNameToken].beginLocation,
					MessageType::Error,
					"The import item shadows an existing member"));

		_targetModule->scope->members[i.first] = std::make_shared<AliasNode>(lexer->tokens[i.second.idxNameToken].beginLocation, this, i.first, i.second.ref);
	}

	for (auto &i : _targetModule->unnamedImports) {
		// Skip bad references.
		// if (!isCompleteIdRef(i.ref))
		//	continue;

		_write(os, (uint32_t)0);
		compileIdRef(os, i.ref);

		importModule(i.ref);
	}

	popMajorContext();

	curMajorContext = MajorContext();
	compileScope(is, os, _targetModule->scope);

#if SLKC_WITH_LANGUAGE_SERVER
	if (isImport)
		tokenInfos = savedTokenInfos;
#endif
}

void Compiler::compileScope(std::istream &is, std::ostream &os, std::shared_ptr<Scope> scope) {
	std::unordered_map<std::string, std::shared_ptr<VarNode>> vars;
	std::unordered_map<std::string, std::shared_ptr<FnNode>> funcs;
	std::unordered_map<std::string, std::shared_ptr<CompiledFnNode>> compiledFuncs;
	std::unordered_map<std::string, std::shared_ptr<ClassNode>> classes;
	std::unordered_map<std::string, std::shared_ptr<InterfaceNode>> interfaces;
	std::unordered_map<std::string, std::shared_ptr<TraitNode>> traits;

	curMajorContext.curMinorContext.curScope = scope;

	for (auto &i : scope->members) {
		switch (i.second->getNodeType()) {
			case NodeType::Var: {
				if (i.second->isImported)
					continue;

				auto m = std::static_pointer_cast<VarNode>(i.second);
				vars[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				if (m->idxNameToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxNameToken];
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.semanticType = SemanticType::Var;
				}

				if (m->idxColonToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxColonToken];
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.completionContext = CompletionContext::Type;
				}
				if (m->type)
					updateCompletionContext(m->type, CompletionContext::Type);
#endif
				break;
			}
			case NodeType::Fn: {
				if (i.second->isImported)
					continue;

				auto m = std::static_pointer_cast<FnNode>(i.second);
				funcs[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				for (auto &j : m->overloadingRegistries) {
					if (j->idxNameToken != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j->idxNameToken];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								j->genericParams,
								j->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::Fn;
					}

					if (j->idxParamLParentheseToken != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j->idxParamLParentheseToken];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								j->genericParams,
								j->genericParamIndices,
								{},
								{});
						tokenInfo.completionContext = CompletionContext::Type;
					}

					if (j->idxReturnTypeColonToken != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j->idxReturnTypeColonToken];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								j->genericParams,
								j->genericParamIndices,
								{},
								{});
						tokenInfo.completionContext = CompletionContext::Type;
					}

					if (j->returnType)
						updateCompletionContext(j->returnType, CompletionContext::Type);

					for (auto &k : j->params) {
						if (k->idxNameToken != SIZE_MAX) {
							// Update corresponding semantic information.
							auto &tokenInfo = tokenInfos[k->idxNameToken];
							tokenInfo.tokenContext =
								TokenContext(
									{},
									curMajorContext.curMinorContext.curScope,
									j->genericParams,
									j->genericParamIndices,
									{},
									{});
							tokenInfo.semanticType = SemanticType::Param;
						}

						updateCompletionContext(k->type, CompletionContext::Type);
						updateSemanticType(k->type, SemanticType::Type);

						// Resolve the type name to fill corresponding `curScope` field in token contexts for completion.
						if (k->type->getTypeId() == Type::Custom)
							resolveCustomTypeName((CustomTypeNameNode *)k->type.get());
					}

					for (auto &k : j->idxParamCommaTokens) {
						if (k != SIZE_MAX) {
							// Update corresponding semantic information.
							auto &tokenInfo = tokenInfos[k];
							tokenInfo.tokenContext =
								TokenContext(
									{},
									curMajorContext.curMinorContext.curScope,
									j->genericParams,
									j->genericParamIndices,
									{},
									{});
							updateCompletionContext(k, CompletionContext::Type);
						}
					}

					for (auto &k : j->genericParams) {
						if (k->idxNameToken != SIZE_MAX) {
							// Update corresponding semantic information.
							auto &tokenInfo = tokenInfos[k->idxNameToken];
							tokenInfo.tokenContext =
								TokenContext(
									{},
									curMajorContext.curMinorContext.curScope,
									j->genericParams,
									j->genericParamIndices,
									{},
									{});
							tokenInfo.semanticType = SemanticType::TypeParam;
						}
					}

					j->updateParamIndices();
				}
#endif
				break;
			}
			case NodeType::Class: {
				if (i.second->isImported)
					continue;

				auto m = std::static_pointer_cast<ClassNode>(i.second);
				classes[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				if (m->idxNameToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxNameToken];
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.semanticType = SemanticType::Class;
				}

				if (m->idxParentSlotLParentheseToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxParentSlotLParentheseToken];
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.completionContext = CompletionContext::Type;
				}

				if (m->idxImplInterfacesColonToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxImplInterfacesColonToken];
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.completionContext = CompletionContext::Type;
				}

				for (auto j : m->idxImplInterfacesCommaTokens) {
					if (j != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.completionContext = CompletionContext::Type;
					}
				}

				for (auto &j : m->genericParams) {
					if (j->idxNameToken != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j->idxNameToken];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::TypeParam;
					}
				}
#endif
				break;
			}
			case NodeType::Interface: {
				if (i.second->isImported)
					continue;

				auto m = std::static_pointer_cast<InterfaceNode>(i.second);
				interfaces[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				if (m->idxNameToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxNameToken];
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.semanticType = SemanticType::Interface;
				}

				for (auto &j : m->genericParams) {
					if (j->idxNameToken != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j->idxNameToken];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::TypeParam;
					}
				}
#endif
				break;
			}
			case NodeType::Trait: {
				if (i.second->isImported)
					continue;

				auto m = std::static_pointer_cast<TraitNode>(i.second);
				traits[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				if (m->idxNameToken != SIZE_MAX) {
					// Update corresponding semantic information.
					auto &tokenInfo = tokenInfos[m->idxNameToken];
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.semanticType = SemanticType::Interface;
				}

				for (auto &j : m->genericParams) {
					if (j->idxNameToken != SIZE_MAX) {
						// Update corresponding semantic information.
						auto &tokenInfo = tokenInfos[j->idxNameToken];
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::TypeParam;
					}
				}
#endif
				break;
			}
			case NodeType::Alias:
			case NodeType::Module:
				break;
			default:
				assert(false);
		}
	}

	for (auto &i : classes)
		verifyInheritanceChain(i.second.get());

	for (auto &i : interfaces)
		verifyInheritanceChain(i.second.get());

	for (auto &i : traits)
		verifyInheritanceChain(i.second.get());

	_write<uint32_t>(os, (uint32_t)vars.size());
	for (auto &i : vars) {
		slxfmt::VarDesc vad = {};

		//
		// Check if the member will shadow member(s) from parent scopes.
		//
		switch (scope->owner->getNodeType()) {
			case NodeType::Class: {
				auto j = (ClassNode *)(scope->owner);

				while (j->parentClass) {
					j = (ClassNode *)resolveCustomTypeName((CustomTypeNameNode *)j->parentClass.get()).get();
					if (j->scope->members.count(i.first))
						throw FatalCompilationError(
							Message(
								i.second->getLocation(),
								MessageType::Error,
								"The member shadows another members from a parent"));
				};

				break;
			}
		}

		if (i.second->access & ACCESS_PUB)
			vad.flags |= slxfmt::VAD_PUB;
		if (i.second->access & ACCESS_STATIC)
			vad.flags |= slxfmt::VAD_STATIC;
		if (i.second->access & ACCESS_FINAL)
			vad.flags |= slxfmt::VAD_FINAL;
		if (i.second->access & ACCESS_NATIVE)
			vad.flags |= slxfmt::VAD_NATIVE;
		if (i.second->initValue)
			vad.flags |= slxfmt::VAD_INIT;

		vad.lenName = (uint8_t)i.first.length();
		_write(os, vad);
		_write(os, i.first.data(), i.first.length());

		compileTypeName(os, i.second->type ? i.second->type : std::make_shared<AnyTypeNameNode>(Location(), SIZE_MAX));

		if (i.second->initValue) {
			if (auto ce = evalConstExpr(i.second->initValue); ce)
				compileExpr(i.second->initValue);
			else
				throw FatalCompilationError(
					Message(
						i.second->initValue->getLocation(),
						MessageType::Error,
						"Expecting a compiling-time expression"));
		}
	}

	auto mergeGenericParams = [this](const GenericParamNodeList &newParams) {
		for (auto &i : newParams) {
			if (curMajorContext.genericParamIndices.count(i->name))
				throw FatalCompilationError(
					Message(
						i->getLocation(),
						MessageType::Error,
						"This generic parameter shadows another generic parameter"));
			curMajorContext.genericParams.push_back(i);
			curMajorContext.genericParamIndices[i->name] = curMajorContext.genericParams.size() - 1;
		}
	};

	for (auto &i : funcs) {
		for (auto &j : i.second->overloadingRegistries) {
			pushMajorContext();

			mergeGenericParams(j->genericParams);

			std::string mangledFnName = i.first;

			{
				std::deque<std::shared_ptr<TypeNameNode>> argTypes;

				argTypes.resize(j->params.size());
				for (size_t k = 0; k < j->params.size(); ++k) {
					argTypes[k] = j->params[k]->type;
				}

				mangledFnName = mangleName(i.first, argTypes, false);
			}

			if (compiledFuncs.count(mangledFnName)) {
				throw FatalCompilationError(
					Message(
						j->loc,
						MessageType::Error,
						"Duplicated function overloading"));
			}

			auto compiledFn = std::make_shared<CompiledFnNode>(j->loc, mangledFnName);
			compiledFn->returnType = j->returnType
										 ? j->returnType
										 : std::static_pointer_cast<TypeNameNode>(std::make_shared<VoidTypeNameNode>(Location(), SIZE_MAX));
			compiledFn->params = j->params;
			compiledFn->paramIndices = j->paramIndices;
			compiledFn->genericParams = j->genericParams;
			compiledFn->genericParamIndices = j->genericParamIndices;
			compiledFn->access = j->access;

			compiledFn->isAsync = j->isAsync;

			curFn = compiledFn;

			if (j->body)
				compileStmt(j->body);

			compiledFuncs[mangledFnName] = compiledFn;

			popMajorContext();
		}
	}

	_write(os, (uint32_t)compiledFuncs.size());
	for (auto &i : compiledFuncs) {
		slxfmt::FnDesc fnd = {};
		bool hasVarArg = false;

		if (i.second->access & ACCESS_PUB)
			fnd.flags |= slxfmt::FND_PUB;
		if (i.second->access & ACCESS_STATIC)
			fnd.flags |= slxfmt::FND_STATIC;
		if (i.second->access & ACCESS_NATIVE)
			fnd.flags |= slxfmt::FND_NATIVE;
		if (i.second->access & ACCESS_OVERRIDE)
			fnd.flags |= slxfmt::FND_OVERRIDE;
		if (i.second->access & ACCESS_FINAL)
			fnd.flags |= slxfmt::FND_FINAL;

		if (i.second->isAsync)
			fnd.flags |= slxfmt::FND_ASYNC;

		if (i.second->paramIndices.count("..."))
			hasVarArg = true;

		if (hasVarArg)
			fnd.flags |= slxfmt::FND_VARG;

		fnd.lenName = (uint16_t)i.first.length();
		fnd.lenBody = (uint32_t)i.second->body.size();
		fnd.nParams = (uint8_t)i.second->params.size() - hasVarArg;
		fnd.nSourceLocDescs = (uint32_t)i.second->srcLocDescs.size();
		fnd.nGenericParams = i.second->genericParams.size();

		_write(os, fnd);
		_write(os, i.first.data(), i.first.length());

		compileTypeName(os, i.second->returnType);

		for (size_t j = 0; j < i.second->genericParams.size(); ++j)
			compileGenericParam(os, i.second->genericParams[j]);

		for (size_t j = 0; j < i.second->params.size() - hasVarArg; ++j)
			compileTypeName(os, i.second->params[j]->type);

		for (auto &j : i.second->body) {
			slxfmt::InsHeader ih;
			ih.opcode = j.opcode;

			if (j.operands.size() > 3)
				throw FatalCompilationError(
					Message(
						i.second->getLocation(),
						MessageType::Error,
						"Too many operands"));
			ih.nOperands = (uint8_t)j.operands.size();

			_write(os, ih);

			for (auto &k : j.operands) {
				if (k) {
					if (k->getNodeType() == NodeType::LabelRef) {
						auto &label = std::static_pointer_cast<LabelRefNode>(k)->label;
						if (!i.second->labels.count(label))
							throw FatalCompilationError(
								Message(
									i.second->getLocation(),
									MessageType::Error,
									"Undefined label: " + std::static_pointer_cast<LabelRefNode>(k)->label));
						k = std::make_shared<U32LiteralExprNode>(i.second->getLocation(), i.second->labels.at(label));
					}
				}
				compileValue(os, k);
			}
		}

		for (auto &j : i.second->srcLocDescs) {
			_write(os, j);
		}
	}

	_write(os, (uint32_t)classes.size());
	for (auto &i : classes) {
		pushMajorContext();

		{
			auto thisType = std::make_shared<CustomTypeNameNode>(i.second->getLocation(), getFullName(i.second.get()), this, i.second->scope.get());
			for (auto &j : i.second->genericParams) {
				thisType->ref.back().genericArgs.push_back(std::make_shared<CustomTypeNameNode>(
					i.second->getLocation(),
					IdRef{ { j->getLocation(), SIZE_MAX, j->name, std::deque<std::shared_ptr<TypeNameNode>>{} } },
					this,
					i.second->scope.get()));
			}
			curMajorContext.thisType = thisType;
		}

		// curMajorContext = MajorContext();
		mergeGenericParams(i.second->genericParams);

		slxfmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= slxfmt::CTD_FINAL;
		if (i.second->parentClass)
			ctd.flags |= slxfmt::CTD_DERIVED;
		ctd.nImpls = (uint8_t)i.second->implInterfaces.size();
		ctd.lenName = (uint8_t)i.first.length();
		ctd.nGenericParams = (uint8_t)i.second->genericParams.size();

		_write(os, ctd);
		_write(os, i.first.data(), i.first.length());

		for (auto &j : i.second->genericParams) {
			compileGenericParam(os, j);
		}

		if (i.second->parentClass) {
			compileIdRef(os, getFullName((MemberNode *)resolveCustomTypeName((CustomTypeNameNode *)i.second->parentClass.get()).get()));
		}

		for (auto &j : i.second->implInterfaces)
			compileIdRef(os, std::static_pointer_cast<CustomTypeNameNode>(j)->ref);

		compileScope(is, os, i.second->scope);

		popMajorContext();
	}

	_write(os, (uint32_t)interfaces.size());
	for (auto &i : interfaces) {
		pushMajorContext();

		// curMajorContext = MajorContext();
		mergeGenericParams(i.second->genericParams);

		slxfmt::InterfaceTypeDesc itd = {};

		if (i.second->access & ACCESS_PUB)
			itd.flags |= slxfmt::ITD_PUB;

		itd.nParents = (uint8_t)i.second->parentInterfaces.size();
		itd.nGenericParams = i.second->genericParams.size();

		itd.lenName = (uint8_t)i.first.length();

		_write(os, itd);
		_write(os, i.first.data(), i.first.length());

		for (auto &j : i.second->genericParams) {
			compileGenericParam(os, j);
		}

		for (auto j : i.second->parentInterfaces) {
			compileIdRef(os, std::static_pointer_cast<CustomTypeNameNode>(j)->ref);
		}

		compileScope(is, os, i.second->scope);

		popMajorContext();
	}

	_write(os, (uint32_t)traits.size());
	for (auto &i : traits) {
		pushMajorContext();

		// curMajorContext = MajorContext();
		mergeGenericParams(i.second->genericParams);

		slxfmt::TraitTypeDesc ttd = {};

		if (i.second->access & ACCESS_PUB)
			ttd.flags |= slxfmt::TTD_PUB;

		ttd.nParents = (uint8_t)i.second->parentTraits.size();

		ttd.lenName = (uint8_t)i.first.length();
		ttd.nGenericParams = i.second->genericParams.size();

		_write(os, ttd);
		_write(os, i.first.data(), i.first.length());

		for (auto &j : i.second->genericParams) {
			compileGenericParam(os, j);
		}

		for (auto j : i.second->parentTraits) {
			compileIdRef(os, std::static_pointer_cast<CustomTypeNameNode>(j)->ref);
		}

		compileScope(is, os, i.second->scope);

		popMajorContext();
	}
}

void Compiler::compileTypeName(std::ostream &fs, std::shared_ptr<TypeNameNode> typeName) {
	switch (typeName->getTypeId()) {
		case Type::I8: {
			_write(fs, slxfmt::Type::I8);
			break;
		}
		case Type::I16: {
			_write(fs, slxfmt::Type::I16);
			break;
		}
		case Type::I32: {
			_write(fs, slxfmt::Type::I32);
			break;
		}
		case Type::I64: {
			_write(fs, slxfmt::Type::I64);
			break;
		}
		case Type::U8: {
			_write(fs, slxfmt::Type::U8);
			break;
		}
		case Type::U16: {
			_write(fs, slxfmt::Type::U16);
			break;
		}
		case Type::U32: {
			_write(fs, slxfmt::Type::U32);
			break;
		}
		case Type::U64: {
			_write(fs, slxfmt::Type::U64);
			break;
		}
		case Type::F32: {
			_write(fs, slxfmt::Type::F32);
			break;
		}
		case Type::F64: {
			_write(fs, slxfmt::Type::F64);
			break;
		}
		case Type::Bool: {
			_write(fs, slxfmt::Type::Bool);
			break;
		}
		case Type::String: {
			_write(fs, slxfmt::Type::String);
			break;
		}
		case Type::Void: {
			_write(fs, slxfmt::Type::None);
			break;
		}
		case Type::Any: {
			_write(fs, slxfmt::Type::Any);
			break;
		}
		case Type::Array: {
			_write(fs, slxfmt::Type::Array);
			compileTypeName(fs, std::static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType);
			break;
		}
		case Type::Fn: {
			// stub
			break;
		}
		case Type::Custom: {
			auto dest = resolveCustomTypeName((CustomTypeNameNode *)typeName.get());

			if (dest->getNodeType() == NodeType::GenericParam) {
				_write(fs, slxfmt::Type::GenericArg);

				auto d = std::static_pointer_cast<GenericParamNode>(dest);
				_write(fs, (uint8_t)d->name.length());
				fs.write(d->name.c_str(), d->name.length());
			} else {
				_write(fs, slxfmt::Type::Object);
				compileIdRef(fs, getFullName((MemberNode *)dest.get()));
			}
			break;
		}
		case Type::Bad:
			break;
		default:
			assert(false);
	}
}

void Compiler::compileIdRef(std::ostream &fs, const IdRef &ref) {
	for (size_t i = 0; i < ref.size(); ++i) {
		slxfmt::IdRefEntryDesc rsd = {};

		auto &entry = ref[i];

		if (i + 1 < ref.size())
			rsd.flags |= slxfmt::RSD_NEXT;

		rsd.lenName = entry.name.size();
		rsd.nGenericArgs = entry.genericArgs.size();
		_write(fs, rsd);
		_write(fs, entry.name.data(), entry.name.length());

		for (auto &j : entry.genericArgs)
			compileTypeName(fs, j);
	}
}

void Compiler::compileValue(std::ostream &fs, std::shared_ptr<AstNode> value) {
	slxfmt::ValueDesc vd = {};

	if (!value) {
		vd.type = slxfmt::Type::None;
		_write(fs, vd);
		return;
	}

	switch (value->getNodeType()) {
		case NodeType::TypeName:
			vd.type = slxfmt::Type::TypeName;
			_write(fs, vd);

			compileTypeName(fs, std::static_pointer_cast<TypeNameNode>(value));
			break;
		case NodeType::ArgRef: {
			auto v = std::static_pointer_cast<ArgRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::ArgValue : slxfmt::Type::Arg;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case NodeType::LocalVarRef: {
			auto v = std::static_pointer_cast<LocalVarRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::LocalVarValue : slxfmt::Type::LocalVar;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case NodeType::RegRef: {
			auto v = std::static_pointer_cast<RegRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::RegValue : slxfmt::Type::Reg;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case NodeType::Expr: {
			std::shared_ptr<ExprNode> expr = std::static_pointer_cast<ExprNode>(value);
			switch (expr->getExprType()) {
				case ExprType::I8: {
					vd.type = slxfmt::Type::I8;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<I8LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I16: {
					vd.type = slxfmt::Type::I16;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<I16LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I32: {
					vd.type = slxfmt::Type::I32;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<I32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I64: {
					vd.type = slxfmt::Type::I64;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<I64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U8: {
					vd.type = slxfmt::Type::U8;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<U8LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U16: {
					vd.type = slxfmt::Type::U16;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<U16LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U32: {
					vd.type = slxfmt::Type::U32;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<U32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U64: {
					vd.type = slxfmt::Type::U64;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<U64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::F32: {
					vd.type = slxfmt::Type::F32;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<F32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::F64: {
					vd.type = slxfmt::Type::F64;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<F64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::Bool: {
					vd.type = slxfmt::Type::Bool;
					_write(fs, vd);

					_write(fs, std::static_pointer_cast<BoolLiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::String: {
					vd.type = slxfmt::Type::String;
					_write(fs, vd);

					auto &s = std::static_pointer_cast<StringLiteralExprNode>(expr)->data;

					_write(fs, (uint32_t)s.length());
					_write(fs, s.data(), s.size());
					break;
				}
				case ExprType::IdRef: {
					vd.type = slxfmt::Type::IdRef;
					_write(fs, vd);

					compileIdRef(fs, std::static_pointer_cast<IdRefExprNode>(expr)->ref);
					break;
				}
				default:
					assert(false);
			}
			break;
		}
		default:
			assert(false);
	}
}

void slake::slkc::Compiler::compileGenericParam(std::ostream &fs, std::shared_ptr<GenericParamNode> genericParam) {
	slxfmt::GenericParamDesc gpd;

	gpd.lenName = (uint8_t)genericParam->name.size();
	gpd.hasBaseType = (bool)genericParam->baseType;
	gpd.nInterfaces = (uint8_t)genericParam->interfaceTypes.size();
	gpd.nTraits = (uint8_t)genericParam->traitTypes.size();

	_write(fs, gpd);

	fs.write(genericParam->name.c_str(), genericParam->name.size());

	if (genericParam->baseType)
		compileTypeName(fs, genericParam->baseType);

	for (auto i : genericParam->interfaceTypes)
		compileTypeName(fs, i);

	for (auto i : genericParam->traitTypes)
		compileTypeName(fs, i);
}

void slake::slkc::Compiler::reset() {
	curMajorContext = MajorContext();
	curFn.reset();

	_rootScope = std::make_shared<Scope>();
	_targetModule.reset();
	_rt = std::make_unique<Runtime>(RT_NOJIT);
	_savedMajorContexts.clear();

	importedDefinitions.clear();
	importedModules.clear();

#if SLKC_WITH_LANGUAGE_SERVER
	tokenInfos.clear();
#endif
	messages.clear();
	modulePaths.clear();
	options = CompilerOptions();
	flags = 0;
	lexer.reset();

	_genericCacheDir.clear();
}
