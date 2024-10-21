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

void Compiler::reloadDoc(const std::string &docName) {
	auto &doc = sourceDocs.at(docName);

	doc->targetModule->scope->parent->members.erase(doc->targetModule->getName().name);

	for (auto &i : doc->targetModule->imports) {
		if (auto it = importedModules.find(i.second.ref->entries); it != importedModules.end()) {
			std::string namedImportDocName = it->second.docName;
			importedModules.erase(it->first);
			reloadDoc(namedImportDocName);
		}
	}

	for (auto &i : doc->targetModule->unnamedImports) {
		if (auto it = importedModules.find(i.ref->entries); it != importedModules.end()) {
			std::string importDocName = it->second.docName;
			importedModules.erase(it);
			reloadDoc(docName);
		}
	}

	doc->lexer = std::make_unique<Lexer>();
	doc->targetModule = std::make_shared<ModuleNode>(this);
}

void Compiler::pushMessage(const std::string &docName, const Message &message) {
	sourceDocs.at(docName)->messages.push_back(message);
}

std::shared_ptr<Scope> slake::slkc::Compiler::completeModuleNamespaces(std::shared_ptr<IdRefNode> ref) {
	auto scope = _rootScope;

	for (size_t i = 0; i < ref->entries.size(); ++i) {
		std::string name = ref->entries[i].name;

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(ref->entries[i].idxToken, [i](TokenInfo &tokenInfo) {
			tokenInfo.semanticType = i == 0 ? SemanticType::Var : SemanticType::Property;
		});
#endif

		if (auto it = scope->members.find(name); it != scope->members.end()) {
			switch (it->second->getNodeType()) {
				case NodeType::Class:
					scope = std::static_pointer_cast<ClassNode>(it->second)->scope;
					break;
				case NodeType::Interface:
					scope = std::static_pointer_cast<InterfaceNode>(it->second)->scope;
					break;
				case NodeType::Module:
					scope = std::static_pointer_cast<ModuleNode>(it->second)->scope;
					break;
				default:
					throw FatalCompilationError(Message{
						tokenRangeToSourceLocation(ref->entries[i].tokenRange),
						MessageType::Error,
						"Cannot import a non-module member" });
			}
		} else {
			auto newMod = std::make_shared<ModuleNode>(this);
			(scope->members[name] = newMod)->bind((MemberNode *)scope->owner);
			newMod->scope->parent = scope.get();
			newMod->moduleName = std::make_shared<IdRefNode>(IdRefEntries{ IdRefEntry({}, SIZE_MAX, name, {}) });
			scope = newMod->scope;
		}
	}

	return scope;
}

void Compiler::scanAndLinkParentFns(Scope *scope, FnNode *fn, const std::string &name) {
	for (AstNode *j = scope->owner; j;) {
		switch (j->getNodeType()) {
			case NodeType::Class: {
				ClassNode *node = (ClassNode *)j;

				if (j != scope->owner) {
					if (auto it = node->scope->members.find(name);
						(it != node->scope->members.end()) && (it->second->getNodeType() == NodeType::Fn)) {
						((FnNode *)fn)->parentFn = (FnNode *)it->second.get();
						goto parentFnScanEnd;
					}
				}

				if (node->parentClass)
					j = resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get()).get();
				else
					goto parentFnScanEnd;

				break;
			}
			default:
				goto parentFnScanEnd;
		}
	}

parentFnScanEnd:;
}

void Compiler::collectMethodsForFulfillmentVerification(std::shared_ptr<Scope> scope, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut) {
	for (auto i : scope->members) {
		if (i.second->getNodeType() == NodeType::Fn) {
			std::shared_ptr<FnNode> m = std::static_pointer_cast<FnNode>(i.second);

			for (auto j : m->overloadingRegistries) {
				if (!j->body) {
					unfilledMethodsOut[i.first].insert(j);
				} else {
					if (auto it = unfilledMethodsOut.find(i.first); it != unfilledMethodsOut.end()) {
						for (auto k : it->second) {
							if (isFnOverloadingDuplicated(j, k)) {
								unfilledMethodsOut[i.first].erase(k);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void Compiler::collectMethodsForFulfillmentVerification(InterfaceNode *node, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut) {
	for (auto i : node->parentInterfaces) {
		collectMethodsForFulfillmentVerification((InterfaceNode *)resolveCustomTypeName((CustomTypeNameNode *)i.get()).get(), unfilledMethodsOut);
	}

	collectMethodsForFulfillmentVerification(node->scope, unfilledMethodsOut);
}

void Compiler::collectMethodsForFulfillmentVerification(ClassNode *node, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut) {
	for (auto i : node->implInterfaces) {
		collectMethodsForFulfillmentVerification((InterfaceNode *)resolveCustomTypeName((CustomTypeNameNode *)i.get()).get(), unfilledMethodsOut);
	}
	if (node->parentClass) {
		collectMethodsForFulfillmentVerification((ClassNode *)resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get()).get(), unfilledMethodsOut);
	}

	collectMethodsForFulfillmentVerification(node->scope, unfilledMethodsOut);
}

void Compiler::verifyIfImplementationFulfilled(std::shared_ptr<ClassNode> node) {
	std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> unfilledMethodsOut;

	collectMethodsForFulfillmentVerification(node.get(), unfilledMethodsOut);

	if (unfilledMethodsOut.size()) {
		pushMessage(
			curDocName,
			Message(
				tokenRangeToSourceLocation({ getCurDoc(), node->idxNameToken }),
				MessageType::Error,
				"Not all abstract methods are implemented"));
	}
}

void Compiler::validateScope(Scope *scope) {
	std::deque<ClassNode *> classes;
	std::deque<InterfaceNode *> interfaces;
	std::deque<FnNode *> funcs;

	pushMajorContext();

	curMajorContext.curMinorContext.curScope = scope->shared_from_this();

	for (auto &i : scope->members) {
		switch (i.second->getNodeType()) {
			case NodeType::Class:
				classes.push_back((ClassNode *)i.second.get());
				break;
			case NodeType::Interface:
				interfaces.push_back((InterfaceNode *)i.second.get());
				break;
			case NodeType::Fn:
				funcs.push_back((FnNode *)i.second.get());
				break;
		}
	}

	// We must make sure the classes and interfaces are valid before we validating their child scope.
	for (auto i : classes) {
		verifyInheritanceChain(i);
		verifyGenericParams(i->genericParams);
	}
	for (auto i : interfaces) {
		verifyInheritanceChain(i);
		verifyGenericParams(i->genericParams);
	}

	// Validate child scopes of classes and interfaces.
	for (auto i : classes)
		validateScope(i->scope.get());
	for (auto i : interfaces)
		validateScope(i->scope.get());

	// Link the functions to their parent functions correctly.
	for (auto i : funcs) {
		scanAndLinkParentFns(scope, i, i->name);
	}

	popMajorContext();
}

void Compiler::compile(std::istream &is, std::ostream &os) {
	auto &doc = sourceDocs.at(curDocName);
	auto _targetModule = doc->targetModule;

	//
	// Clear the previous generic cache.
	// Note that we don't clear the generic cache after every compilation immediately,
	// because this will cause some generic instances referenced as parent value by member values
	// to be expired, this will influence services that analyzes the final compilation status,
	// such as language server.
	//
	_genericCacheDir.clear();

	std::string s;
	{
		is.seekg(0, std::ios::end);
		size_t size = is.tellg();
		s.resize(size);
		is.seekg(0, std::ios::beg);
		is.read(s.data(), size);
	}

	doc->lexer = std::make_unique<Lexer>();
	try {
		doc->lexer->lex(s);
	} catch (LexicalError e) {
		throw FatalCompilationError(
			Message(
				SourceLocation{ e.position, e.position },
				MessageType::Error,
				"Lexical error"));
	}

#if SLKC_WITH_LANGUAGE_SERVER
	doc->tokenInfos.resize(doc->lexer->tokens.size());
#endif

	Parser parser;
	try {
		parser.parse(doc.get(), this);
	} catch (SyntaxError e) {
		throw FatalCompilationError(
			Message(
				tokenRangeToSourceLocation(e.tokenRange),
				MessageType::Error,
				e.what()));
	}

	//
	// Because tokens may be splitted into multiple new tokens during parsing,
	// we have to resize the token information again for the new tokens.
	//
#if SLKC_WITH_LANGUAGE_SERVER
	doc->tokenInfos.resize(doc->lexer->tokens.size());
#endif

#if SLKC_WITH_LANGUAGE_SERVER
	if (_targetModule->moduleName) {
		updateCompletionContext(_targetModule->moduleName, CompletionContext::ModuleName);
		updateSemanticType(_targetModule->moduleName, SemanticType::Var);

		std::shared_ptr<IdRefNode> curRef = std::make_shared<IdRefNode>();
		for (size_t i = 0; i < _targetModule->moduleName->entries.size(); ++i) {
			updateTokenInfo(_targetModule->moduleName->entries[i].idxToken, [&curRef](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.importedPath = curRef->duplicate<IdRefNode>();
			});
			updateTokenInfo(_targetModule->moduleName->entries[i].idxAccessOpToken, [&curRef](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.importedPath = curRef->duplicate<IdRefNode>();
			});

			curRef->entries.push_back(_targetModule->moduleName->entries[i]);
		}
	}
#endif

	{
		slxfmt::ImgHeader ih = {};

		memcpy(ih.magic, slxfmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = (uint16_t)(_targetModule->imports.size() + _targetModule->unnamedImports.size());

		if (_targetModule->moduleName)
			ih.flags |= slxfmt::IMH_MODNAME;

		os.write((char *)&ih, sizeof(ih));
	}

	if (_targetModule->moduleName && !isCompleteIdRef(_targetModule->moduleName->entries)) {
		throw FatalCompilationError(Message(
			tokenRangeToSourceLocation(_targetModule->moduleName->entries.back().tokenRange),
			MessageType::Error,
			"Expecting a complete module name"));
	}

	if (_targetModule->moduleName) {
		auto trimmedModuleName = _targetModule->moduleName->duplicate<IdRefNode>();
		trimmedModuleName->entries.pop_back();

		auto scope = completeModuleNamespaces(trimmedModuleName);

		importedModules[_targetModule->moduleName->entries] = { curDocName };

		(scope->members[_targetModule->moduleName->entries.back().name] = _targetModule)->bind((MemberNode *)scope->owner);
		_targetModule->scope->parent = scope.get();

		compileIdRef(os, _targetModule->moduleName);
	}

	pushMajorContext();

#if SLKC_WITH_LANGUAGE_SERVER
	for (auto &i : _targetModule->imports) {
		updateTokenInfo(i.second.idxNameToken, [this](TokenInfo &tokenInfo) {
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Property;
		});

		updateCompletionContext(i.second.ref, CompletionContext::Import);
		updateSemanticType(i.second.ref, SemanticType::Property);

		std::shared_ptr<IdRefNode> importedPath = std::make_shared<IdRefNode>();
		for (size_t j = 0; j < i.second.ref->entries.size(); ++j) {
			updateTokenInfo(i.second.ref->entries[j].idxAccessOpToken, [&importedPath](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.importedPath = importedPath;
			});
			updateTokenInfo(i.second.ref->entries[j].idxToken, [&importedPath](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.importedPath = importedPath;
			});

			importedPath->entries.push_back(i.second.ref->entries[j]);
		}
	}

	for (auto &i : _targetModule->unnamedImports) {
		updateCompletionContext(i.ref, CompletionContext::Import);
		updateSemanticType(i.ref, SemanticType::Property);

		std::shared_ptr<IdRefNode> importedPath = std::make_shared<IdRefNode>();
		for (size_t j = 0; j < i.ref->entries.size(); ++j) {
			updateTokenInfo(i.ref->entries[j].idxAccessOpToken, [&importedPath](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.importedPath = importedPath;
			});
			updateTokenInfo(i.ref->entries[j].idxToken, [&importedPath](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.importedPath = importedPath;
			});

			importedPath->entries.push_back(i.ref->entries[j]);
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

		pushMajorContext();

		importModule(i.second.ref);

		popMajorContext();

		if (_targetModule->scope->members.count(i.first))
			throw FatalCompilationError(
				Message(
					doc->lexer->tokens[i.second.idxNameToken]->location,
					MessageType::Error,
					"The import item shadows an existing member"));

		_targetModule->scope->members[i.first] = std::make_shared<AliasNode>(this, i.first, i.second.ref);
	}

	for (auto &i : _targetModule->unnamedImports) {
		// Skip bad references.
		// if (!isCompleteIdRef(i.ref))
		//	continue;

		_write(os, (uint32_t)0);
		compileIdRef(os, i.ref);

		pushMajorContext();

		importModule(i.ref);

		popMajorContext();
	}

	popMajorContext();

	validateScope(_targetModule->scope.get());
	curMajorContext = MajorContext();
	compileScope(is, os, _targetModule->scope);
}

void Compiler::compileScope(std::istream &is, std::ostream &os, std::shared_ptr<Scope> scope) {
	std::unordered_map<std::string, std::shared_ptr<VarNode>> vars;
	std::unordered_map<std::string, std::shared_ptr<FnNode>> funcs;
	std::unordered_map<std::string, std::shared_ptr<ClassNode>> classes;
	std::unordered_map<std::string, std::shared_ptr<InterfaceNode>> interfaces;

	curMajorContext.curMinorContext.curScope = scope;

	for (auto &i : scope->members) {
		switch (i.second->getNodeType()) {
			case NodeType::Var: {
				auto m = std::static_pointer_cast<VarNode>(i.second);
				vars[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(m->idxNameToken, [this](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.semanticType = SemanticType::Var;
				});

				updateTokenInfo(m->idxColonToken, [this](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.completionContext = CompletionContext::Type;
				});

				if (m->type)
					updateCompletionContext(m->type, CompletionContext::Type);
#endif
				break;
			}
			case NodeType::Fn: {
				auto m = std::static_pointer_cast<FnNode>(i.second);
				funcs[i.first] = m;

				for (auto &j : m->overloadingRegistries) {
#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(j->idxNameToken, [this, &j](TokenInfo &tokenInfo) {
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								j->genericParams,
								j->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::Fn;
					});

					updateTokenInfo(j->idxParamLParentheseToken, [this, &j](TokenInfo &tokenInfo) {
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								j->genericParams,
								j->genericParamIndices,
								{},
								{});
						tokenInfo.completionContext = CompletionContext::Type;
					});

					updateTokenInfo(j->idxReturnTypeColonToken, [this, &j](TokenInfo &tokenInfo) {
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								j->genericParams,
								j->genericParamIndices,
								{},
								{});
						tokenInfo.completionContext = CompletionContext::Type;
					});

					if (j->returnType)
						updateCompletionContext(j->returnType, CompletionContext::Type);

					for (auto &k : j->params) {
						updateTokenInfo(k->idxNameToken, [this, &j](TokenInfo &tokenInfo) {
							tokenInfo.tokenContext =
								TokenContext(
									{},
									curMajorContext.curMinorContext.curScope,
									j->genericParams,
									j->genericParamIndices,
									{},
									{});
							tokenInfo.semanticType = SemanticType::Param;
						});

						if (k->type) {
							updateCompletionContext(k->type, CompletionContext::Type);
							updateSemanticType(k->type, SemanticType::Type);

							// Resolve the type name to fill corresponding `curScope` field in token contexts for completion.
							if (k->type->getTypeId() == TypeId::Custom)
								resolveCustomTypeName((CustomTypeNameNode *)k->type.get());
						}
					}

					for (auto &k : j->idxParamCommaTokens) {
						updateTokenInfo(k, [this, &j, &k](TokenInfo &tokenInfo) {
							tokenInfo.tokenContext =
								TokenContext(
									{},
									curMajorContext.curMinorContext.curScope,
									j->genericParams,
									j->genericParamIndices,
									{},
									{});
							updateCompletionContext(k, CompletionContext::Type);
						});
					}

					for (auto &k : j->genericParams) {
						updateTokenInfo(k->idxNameToken, [this, &j](TokenInfo &tokenInfo) {
							tokenInfo.tokenContext =
								TokenContext(
									{},
									curMajorContext.curMinorContext.curScope,
									j->genericParams,
									j->genericParamIndices,
									{},
									{});
							tokenInfo.semanticType = SemanticType::TypeParam;
						});
					}
#endif

					j->updateParamIndices();
				}
				break;
			}
			case NodeType::Class: {
				auto m = std::static_pointer_cast<ClassNode>(i.second);
				classes[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(m->idxNameToken, [this, &m](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.semanticType = SemanticType::Class;
				});

				updateTokenInfo(m->idxParentSlotLParentheseToken, [this, &m](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.completionContext = CompletionContext::Type;
				});

				if (m->parentClass)
					updateCompletionContext(m->parentClass, CompletionContext::Type);

				updateTokenInfo(m->idxImplInterfacesColonToken, [this, &m](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.completionContext = CompletionContext::Type;
				});

				for (auto j : m->idxImplInterfacesSeparatorTokens) {
					updateTokenInfo(j, [this, &m](TokenInfo &tokenInfo) {
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.completionContext = CompletionContext::Type;
					});
				}

				for (auto &j : m->implInterfaces) {
					updateCompletionContext(j, CompletionContext::Type);
				}

				for (auto &j : m->genericParams) {
					updateTokenInfo(j->idxNameToken, [this, &m](TokenInfo &tokenInfo) {
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::TypeParam;
					});
				}
#endif
				break;
			}
			case NodeType::Interface: {
				auto m = std::static_pointer_cast<InterfaceNode>(i.second);
				interfaces[i.first] = m;

#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(m->idxNameToken, [this, &m](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext =
						TokenContext(
							{},
							curMajorContext.curMinorContext.curScope,
							m->genericParams,
							m->genericParamIndices,
							{},
							{});
					tokenInfo.semanticType = SemanticType::Interface;
				});

				for (auto &j : m->parentInterfaces) {
					updateCompletionContext(j, CompletionContext::Type);
				}

				for (auto &j : m->genericParams) {
					updateTokenInfo(j->idxNameToken, [this, &m](TokenInfo &tokenInfo) {
						tokenInfo.tokenContext =
							TokenContext(
								{},
								curMajorContext.curMinorContext.curScope,
								m->genericParams,
								m->genericParamIndices,
								{},
								{});
						tokenInfo.semanticType = SemanticType::TypeParam;
					});
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

	auto mergeGenericParams = [this](const GenericParamNodeList &newParams) {
		for (auto &i : newParams) {
			if (curMajorContext.genericParamIndices.count(i->name))
				pushMessage(
					curDocName,
					Message(
						tokenRangeToSourceLocation(i->tokenRange),
						MessageType::Error,
						"This generic parameter shadows another generic parameter"));
			curMajorContext.genericParams.push_back(i);
			curMajorContext.genericParamIndices[i->name] = curMajorContext.genericParams.size() - 1;
		}
	};

	//
	// Compile and write classes.
	//
	_write(os, (uint32_t)classes.size());
	for (auto &i : classes) {
		MemberNodeCompilingStatusGuard compilingStatusGuard(i.second);

		pushMajorContext();

		{
			auto thisType = std::make_shared<CustomTypeNameNode>(getFullName(i.second.get()), this, i.second->scope.get());
			for (auto &j : i.second->genericParams) {
				thisType->ref->entries.back().genericArgs.push_back(std::make_shared<CustomTypeNameNode>(
					std::make_shared<IdRefNode>(IdRefEntries{ { j->tokenRange, SIZE_MAX, j->name, std::deque<std::shared_ptr<TypeNameNode>>{} } }),
					this,
					i.second->scope.get()));
			}
			curMajorContext.thisType = thisType;
		}

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

		verifyIfImplementationFulfilled(i.second);

		popMajorContext();
	}

	//
	// Compile and write interfaces.
	//
	_write(os, (uint32_t)interfaces.size());
	for (auto &i : interfaces) {
		MemberNodeCompilingStatusGuard compilingStatusGuard(i.second);

		pushMajorContext();

		mergeGenericParams(i.second->genericParams);

		slxfmt::InterfaceTypeDesc itd = {};

		if (i.second->access & ACCESS_PUB)
			itd.flags |= slxfmt::ITD_PUB;

		itd.nParents = (uint8_t)i.second->parentInterfaces.size();
		itd.nGenericParams = (uint8_t)i.second->genericParams.size();

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

	//
	// Compile and write variables.
	//
	_write<uint32_t>(os, (uint32_t)vars.size());
	for (auto &i : vars) {
		MemberNodeCompilingStatusGuard compilingStatusGuard(i.second);

		slxfmt::VarDesc vad = {};

		//
		// Check if the member will shadow member(s) from parent scopes.
		//
		switch (scope->owner->getNodeType()) {
			case NodeType::Class: {
				auto j = (ClassNode *)(scope->owner);

				while (j->parentClass) {
					j = (ClassNode *)resolveCustomTypeName((CustomTypeNameNode *)j->parentClass.get()).get();
					if (j->scope->members.count(i.first)) {
						pushMessage(
							curDocName,
							Message(
								tokenRangeToSourceLocation(i.second->tokenRange),
								MessageType::Error,
								"The member shadows another member from the parent"));

						goto skipCurVar;
					}
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

		{
			auto varType = i.second->type ? i.second->type : std::make_shared<AnyTypeNameNode>(SIZE_MAX);
			compileTypeName(os, varType);

			if (isLValueType(varType)) {
				pushMessage(
					curDocName,
					Message(
						tokenRangeToSourceLocation(varType->tokenRange),
						MessageType::Error,
						"Cannot use reference types for member variables"));
				goto skipCurVar;
			}

			if (i.second->initValue) {
				if (auto ce = evalConstExpr(i.second->initValue); ce)
					compileValue(os, ce);
				else {
					pushMessage(
						curDocName,
						Message(
							tokenRangeToSourceLocation(i.second->initValue->tokenRange),
							MessageType::Error,
							"Expecting a compiling-time expression"));
					goto skipCurVar;
				}
			}
		}
	skipCurVar:;
	}

	//
	// Compile and write functions.
	//
	_write<uint32_t>(os, (uint32_t)funcs.size());
	for (auto &i : funcs) {
		_write<uint32_t>(os, (uint32_t)i.second->overloadingRegistries.size());

		std::set<std::shared_ptr<FnOverloadingNode>> compiledOverloadingsWithDuplication;

		for (auto &j : i.second->overloadingRegistries) {
			MemberNodeCompilingStatusGuard compilingStatusGuard(j);

			if (i.first == "delete") {
				j->isVirtual = true;
			}

			if (j->isVirtual) {
				if (i.first == "new") {
					pushMessage(
						curDocName,
						Message(
							sourceDocs.at(curDocName)->lexer->tokens[j->idxVirtualModifierToken]->location,
							MessageType::Error,
							"new() method cannot be declared as virtual"));
					goto skipCurOverloading;
				}

				switch (scope->owner->getNodeType()) {
					case NodeType::Class:
					case NodeType::Interface:
						break;
					default:
						pushMessage(
							curDocName,
							Message(
								sourceDocs.at(curDocName)->lexer->tokens[j->idxVirtualModifierToken]->location,
								MessageType::Error,
								"Virtual modifier is invalid in the context"));
						goto skipCurOverloading;
				}

				if (j->genericParams.size()) {
					pushMessage(
						curDocName,
						Message(
							sourceDocs.at(curDocName)->lexer->tokens[j->idxVirtualModifierToken]->location,
							MessageType::Error,
							"Generic method cannot be declared as virtual"));
					goto skipCurOverloading;
				}
			}

			for (size_t k = 0; k < j->specializationArgs.size(); ++k) {
				auto curType = j->specializationArgs[k];

				switch (curType->getTypeId()) {
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
					case TypeId::String:
					case TypeId::Bool:
					case TypeId::Any:
					case TypeId::Void:
					case TypeId::Array:
					case TypeId::Fn:
						break;
					case TypeId::Custom: {
						auto m = resolveCustomTypeName((CustomTypeNameNode *)curType.get());

						if (m->getNodeType() == NodeType::GenericParam) {
							pushMessage(
								curDocName,
								Message(
									tokenRangeToSourceLocation(curType->tokenRange),
									MessageType::Error,
									"Specialization argument must be deterministic"));
							goto skipCurOverloading;
						}

						break;
					}
					default:
						pushMessage(
							curDocName,
							Message(
								tokenRangeToSourceLocation(curType->tokenRange),
								MessageType::Error,
								"Specified type cannot be used as a specialization argument"));
						goto skipCurOverloading;
				}
			}

			pushMajorContext();

			mergeGenericParams(j->genericParams);

			for (auto &k : i.second->overloadingRegistries) {
				if (j == k)
					continue;

				if (isFnOverloadingDuplicated(k, j)) {
					if (compiledOverloadingsWithDuplication.count(k)) {
						pushMessage(
							curDocName,
							Message(
								tokenRangeToSourceLocation(j->tokenRange),
								MessageType::Error,
								"Duplicated function overloading"));
						popMajorContext();
						goto skipCurOverloading;
					}
					compiledOverloadingsWithDuplication.insert(j);
				}
			}

			{
				auto compiledFn = std::make_shared<CompiledFnNode>(i.first);
				compiledFn->tokenRange = j->tokenRange;
				compiledFn->returnType = j->returnType
											 ? j->returnType
											 : std::static_pointer_cast<TypeNameNode>(std::make_shared<VoidTypeNameNode>(SIZE_MAX));
				compiledFn->params = j->params;
				compiledFn->paramIndices = j->paramIndices;
				compiledFn->genericParams = j->genericParams;
				compiledFn->genericParamIndices = j->genericParamIndices;
				compiledFn->access = j->access;

				compiledFn->hasVarArgs = j->isVaridic();
				compiledFn->isAsync = j->isAsync;

				curFn = compiledFn;

				if (j->body)
					compileStmt(j->body);

				slxfmt::FnDesc fnd = {};

				if (compiledFn->access & ACCESS_PUB)
					fnd.flags |= slxfmt::FND_PUB;
				if (compiledFn->access & ACCESS_STATIC)
					fnd.flags |= slxfmt::FND_STATIC;
				if (compiledFn->access & ACCESS_NATIVE)
					fnd.flags |= slxfmt::FND_NATIVE;
				if (compiledFn->access & ACCESS_OVERRIDE)
					fnd.flags |= slxfmt::FND_OVERRIDE;
				if (compiledFn->access & ACCESS_FINAL)
					fnd.flags |= slxfmt::FND_FINAL;

				if (compiledFn->isAsync)
					fnd.flags |= slxfmt::FND_ASYNC;

				if (j->isVirtual)
					fnd.flags |= slxfmt::FND_VIRTUAL;

				if (j->isVaridic())
					fnd.flags |= slxfmt::FND_VARG;

				fnd.lenName = (uint16_t)i.first.length();
				fnd.lenBody = (uint32_t)compiledFn->body.size();
				fnd.nParams = (uint8_t)compiledFn->params.size();
				fnd.nSourceLocDescs = (uint32_t)compiledFn->srcLocDescs.size();
				fnd.nGenericParams = compiledFn->genericParams.size();

				_write(os, fnd);
				_write(os, i.first.data(), i.first.length());

				compileTypeName(os, compiledFn->returnType);

				for (size_t j = 0; j < compiledFn->genericParams.size(); ++j)
					compileGenericParam(os, compiledFn->genericParams[j]);

				for (size_t j = 0; j < compiledFn->params.size(); ++j)
					compileTypeName(os, compiledFn->params[j]->type ? compiledFn->params[j]->type : std::make_shared<AnyTypeNameNode>(SIZE_MAX));

				for (auto &j : compiledFn->body) {
					slxfmt::InsHeader ih;
					ih.opcode = j.opcode;

					if (j.operands.size() > 3) {
						pushMessage(
							curDocName,
							Message(
								tokenRangeToSourceLocation(compiledFn->tokenRange),
								MessageType::Error,
								"Too many operands"));
						break;
					}
					ih.nOperands = (uint8_t)j.operands.size();
					ih.hasOutputOperand = (bool)j.output;

					_write(os, ih);

					if (j.output)
						compileValue(os, j.output);

					for (auto &k : j.operands) {
						if (k) {
							if (k->getNodeType() == NodeType::LabelRef) {
								auto &label = std::static_pointer_cast<LabelRefNode>(k)->label;
								assert(compiledFn->labels.count(label));
								k = std::make_shared<U32LiteralExprNode>(compiledFn->labels.at(label));
							}
						}
						compileValue(os, k);
					}
				}

				for (auto &j : compiledFn->srcLocDescs) {
					_write(os, j);
				}
			}

			popMajorContext();

		skipCurOverloading:;
		}
	}
}

void Compiler::compileTypeName(std::ostream &fs, std::shared_ptr<TypeNameNode> typeName) {
	if (typeName->isRef) {
		_write(fs, slxfmt::TypeId::Ref);

		auto derefTypeName = typeName->duplicate<TypeNameNode>();
		derefTypeName->isRef = false;
		compileTypeName(fs, derefTypeName);
	} else {
		switch (typeName->getTypeId()) {
			case TypeId::I8: {
				_write(fs, slxfmt::TypeId::I8);
				break;
			}
			case TypeId::I16: {
				_write(fs, slxfmt::TypeId::I16);
				break;
			}
			case TypeId::I32: {
				_write(fs, slxfmt::TypeId::I32);
				break;
			}
			case TypeId::I64: {
				_write(fs, slxfmt::TypeId::I64);
				break;
			}
			case TypeId::U8: {
				_write(fs, slxfmt::TypeId::U8);
				break;
			}
			case TypeId::U16: {
				_write(fs, slxfmt::TypeId::U16);
				break;
			}
			case TypeId::U32: {
				_write(fs, slxfmt::TypeId::U32);
				break;
			}
			case TypeId::U64: {
				_write(fs, slxfmt::TypeId::U64);
				break;
			}
			case TypeId::F32: {
				_write(fs, slxfmt::TypeId::F32);
				break;
			}
			case TypeId::F64: {
				_write(fs, slxfmt::TypeId::F64);
				break;
			}
			case TypeId::Bool: {
				_write(fs, slxfmt::TypeId::Bool);
				break;
			}
			case TypeId::String: {
				_write(fs, slxfmt::TypeId::String);
				break;
			}
			case TypeId::Void: {
				_write(fs, slxfmt::TypeId::None);
				break;
			}
			case TypeId::Any: {
				_write(fs, slxfmt::TypeId::Any);
				break;
			}
			case TypeId::Array: {
				auto t = std::static_pointer_cast<ArrayTypeNameNode>(typeName);
				_write(fs, slxfmt::TypeId::Array);
				compileTypeName(fs, t->elementType);
				break;
			}
			case TypeId::Fn: {
				// stub
				break;
			}
			case TypeId::Custom: {
				auto dest = resolveCustomTypeName((CustomTypeNameNode *)typeName.get());

				if (dest->getNodeType() == NodeType::GenericParam) {
					_write(fs, slxfmt::TypeId::GenericArg);

					auto d = std::static_pointer_cast<GenericParamNode>(dest);
					_write(fs, (uint8_t)d->name.length());
					fs.write(d->name.c_str(), d->name.length());
				} else {
					_write(fs, slxfmt::TypeId::Object);
					compileIdRef(fs, getFullName((MemberNode *)dest.get()));
				}
				break;
			}
			case TypeId::Bad:
				break;
			default:
				assert(false);
		}
	}
}

void Compiler::compileIdRef(std::ostream &fs, std::shared_ptr<IdRefNode> ref) {
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		slxfmt::IdRefEntryDesc rsd = {};

		auto &entry = ref->entries[i];

		if (i + 1 < ref->entries.size())
			rsd.flags |= slxfmt::RSD_NEXT;

		rsd.lenName = entry.name.size();
		rsd.nGenericArgs = entry.genericArgs.size();
		if (entry.hasParamTypes) {
			rsd.flags |= slxfmt::RSD_HASARG;
			rsd.nParams = entry.paramTypes.size();
			if (entry.hasVarArg)
				rsd.flags |= slxfmt::RSD_VARARG;
		} else {
			rsd.nParams = 0;
		}
		_write(fs, rsd);
		_write(fs, entry.name.data(), entry.name.length());

		for (auto j : entry.genericArgs)
			compileTypeName(fs, j);

		if (entry.hasParamTypes) {
			for (auto j : entry.paramTypes)
				compileTypeName(fs, j);
		}
	}
}

void Compiler::compileValue(std::ostream &fs, std::shared_ptr<AstNode> value) {
	if (!value) {
		_write(fs, slxfmt::TypeId::None);
		return;
	}

	switch (value->getNodeType()) {
		case NodeType::TypeName:
			_write(fs, slxfmt::TypeId::TypeName);

			compileTypeName(fs, std::static_pointer_cast<TypeNameNode>(value));
			break;
		case NodeType::RegRef: {
			auto v = std::static_pointer_cast<RegRefNode>(value);
			_write(fs, slxfmt::TypeId::Reg);

			_write(fs, v->index);
			break;
		}
		case NodeType::Expr: {
			std::shared_ptr<ExprNode> expr = std::static_pointer_cast<ExprNode>(value);
			switch (expr->getExprType()) {
				case ExprType::I8: {
					_write(fs, slxfmt::TypeId::I8);

					_write(fs, std::static_pointer_cast<I8LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I16: {
					_write(fs, slxfmt::TypeId::I16);

					_write(fs, std::static_pointer_cast<I16LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I32: {
					_write(fs, slxfmt::TypeId::I32);

					_write(fs, std::static_pointer_cast<I32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I64: {
					_write(fs, slxfmt::TypeId::I64);

					_write(fs, std::static_pointer_cast<I64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U8: {
					_write(fs, slxfmt::TypeId::U8);

					_write(fs, std::static_pointer_cast<U8LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U16: {
					_write(fs, slxfmt::TypeId::U16);

					_write(fs, std::static_pointer_cast<U16LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U32: {
					_write(fs, slxfmt::TypeId::U32);

					_write(fs, std::static_pointer_cast<U32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U64: {
					_write(fs, slxfmt::TypeId::U64);

					_write(fs, std::static_pointer_cast<U64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::F32: {
					_write(fs, slxfmt::TypeId::F32);

					_write(fs, std::static_pointer_cast<F32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::F64: {
					_write(fs, slxfmt::TypeId::F64);

					_write(fs, std::static_pointer_cast<F64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::Bool: {
					_write(fs, slxfmt::TypeId::Bool);

					_write(fs, std::static_pointer_cast<BoolLiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::String: {
					_write(fs, slxfmt::TypeId::String);

					auto s = std::static_pointer_cast<StringLiteralExprNode>(expr)->data;

					_write(fs, (uint32_t)s.length());
					_write(fs, s.data(), s.size());
					break;
				}
				case ExprType::IdRef: {
					_write(fs, slxfmt::TypeId::IdRef);

					compileIdRef(fs, std::static_pointer_cast<IdRefExprNode>(expr)->ref);
					break;
				}
				case ExprType::Array: {
					_write(fs, slxfmt::TypeId::Array);

					auto a = std::static_pointer_cast<ArrayExprNode>(expr);

					compileTypeName(fs, a->evaluatedElementType);

					_write(fs, (uint32_t)a->elements.size());

					for (auto i : a->elements)
						compileValue(fs, i);

					break;
				}
				case ExprType::Null: {
					_write(fs, slxfmt::TypeId::None);
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

	_write(fs, gpd);

	fs.write(genericParam->name.c_str(), genericParam->name.size());

	if (genericParam->baseType)
		compileTypeName(fs, genericParam->baseType);

	for (auto i : genericParam->interfaceTypes)
		compileTypeName(fs, i);
}

void slake::slkc::Compiler::reload() {
	curMajorContext = MajorContext();
	curFn.reset();

	// _rootScope = std::make_shared<Scope>();
	associatedRuntime = std::make_unique<Runtime>(std::pmr::get_default_resource(), RT_NOJIT);
	_savedMajorContexts.clear();

	importedDefinitions.clear();
	// importedModules.clear();

	// sourceDocs.clear();
	// modulePaths.clear();
	options = CompilerOptions();
	flags = 0;

	_genericCacheDir.clear();
}
