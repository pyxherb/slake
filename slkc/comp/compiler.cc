#include "compiler.h"

using namespace slkc;

SLKC_API CompilationContext::CompilationContext(CompilationContext *parent) : parent(parent) {
}
SLKC_API CompilationContext::~CompilationContext() {
}

SLKC_API AstNodePtr<VarNode> CompilationContext::lookupLocalVar(const std::string_view &name) {
	for (CompilationContext *i = this; i; i = i->parent) {
		AstNodePtr<VarNode> varNode = i->getLocalVar(name);

		if (varNode) {
			return varNode;
		}
	}
	return {};
}

SLKC_API NormalCompilationContext::BlockLayer::~BlockLayer() {
}

SLKC_API NormalCompilationContext::NormalCompilationContext(CompileEnvironment *compileEnv, CompilationContext *parent) : CompilationContext(parent), allocator(compileEnv->allocator), savedBlockLayers(compileEnv->allocator.get()), curBlockLayer(compileEnv->allocator.get()), labels(compileEnv->allocator.get()), labelNameIndices(compileEnv->allocator.get()), generatedInstructions(compileEnv->allocator.get()), document(compileEnv->document), baseBlockLevel(parent ? parent->getBlockLevel() : 0), baseInsOff(parent ? parent->getCurInsOff() : 0) {
}
SLKC_API NormalCompilationContext::~NormalCompilationContext() {
}

SLKC_API std::optional<CompilationError> NormalCompilationContext::allocLabel(uint32_t &labelIdOut) {
	peff::SharedPtr<Label> label = peff::makeShared<Label>(allocator.get(), peff::String(allocator.get()));

	if (!label) {
		return genOutOfMemoryCompError();
	}

	labelIdOut = labels.size();

	if (!labels.pushBack(peff::SharedPtr<Label>(label))) {
		return genOutOfMemoryCompError();
	}

	return {};
}
SLKC_API void NormalCompilationContext::setLabelOffset(uint32_t labelId, uint32_t offset) const {
	labels.at(labelId)->offset = offset;
}
SLKC_API std::optional<CompilationError> NormalCompilationContext::setLabelName(uint32_t labelId, const std::string_view &name) {
	if (!labels.at(labelId)->name.build(name)) {
		return genOutOfMemoryCompError();
	}
	return {};
}
SLKC_API uint32_t NormalCompilationContext::getLabelOffset(uint32_t labelId) {
	return labels.at(labelId)->offset;
}

SLKC_API std::optional<CompilationError> NormalCompilationContext::allocReg(uint32_t &regOut) {
	if (nTotalRegs < UINT32_MAX) {
		regOut = nTotalRegs++;
		return {};
	}

	return CompilationError({ 0 }, CompilationErrorKind::RegLimitExceeded);
}

SLKC_API std::optional<CompilationError> NormalCompilationContext::emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) {
	slake::Instruction insOut;

	insOut.opcode = opcode;
	insOut.output = outputRegIndex;
	if (!insOut.reserveOperands(allocator.get(), operands.size())) {
		return genOutOfMemoryCompError();
	}

	auto it = operands.begin();
	for (size_t i = 0; i < operands.size(); ++i) {
		insOut.operands[i] = *it++;
	}

	if (!generatedInstructions.pushBack(std::move(insOut))) {
		return genOutOfRuntimeMemoryCompError();
	}

	return {};
}

SLKC_API std::optional<CompilationError> NormalCompilationContext::emitIns(slake::Opcode opcode, uint32_t outputRegIndex, slake::Value *operands, size_t nOperands)
{
	slake::Instruction insOut;

	insOut.opcode = opcode;
	insOut.output = outputRegIndex;
	if (!insOut.reserveOperands(allocator.get(), nOperands)) {
		return genOutOfMemoryCompError();
	}

	memcpy(insOut.operands, operands, sizeof(slake::Value) * nOperands);

	if (!generatedInstructions.pushBack(std::move(insOut))) {
		return genOutOfRuntimeMemoryCompError();
	}

	return {};
}

SLKC_API std::optional<CompilationError> NormalCompilationContext::allocLocalVar(const TokenRange &tokenRange, const std::string_view &name, uint32_t reg, AstNodePtr<TypeNameNode> type, AstNodePtr<VarNode> &localVarOut) {
	AstNodePtr<VarNode> newVar;

	if (!(newVar = makeAstNode<VarNode>(allocator.get(), allocator.get(), document))) {
		return genOutOfMemoryCompError();
	}

	if (!newVar->name.build(name)) {
		return genOutOfMemoryCompError();
	}

	newVar->type = type;

	newVar->idxReg = reg;

	if (!curBlockLayer.localVars.insert(newVar->name, AstNodePtr<VarNode>(newVar))) {
		return genOutOfMemoryCompError();
	}

	localVarOut = newVar;

	return {};
}
SLKC_API AstNodePtr<VarNode> NormalCompilationContext::getLocalVarInCurLevel(const std::string_view &name) {
	if (auto it = curBlockLayer.localVars.find(name); it != curBlockLayer.localVars.end()) {
		return it.value();
	}

	return {};
}
SLKC_API AstNodePtr<VarNode> NormalCompilationContext::getLocalVar(const std::string_view &name) {
	if (auto v = getLocalVarInCurLevel(name); v)
		return v;

	for (auto i = savedBlockLayers.beginReversed(); i != savedBlockLayers.endReversed(); ++i) {
		if (auto it = i->localVars.find(name); it != i->localVars.end()) {
			return it.value();
		}
	}

	return {};
}

SLKC_API void NormalCompilationContext::setBreakLabel(uint32_t labelId, uint32_t blockLevel) {
	breakStmtJumpDestLabel = labelId;
	breakStmtBlockLevel = blockLevel;
}
SLKC_API void NormalCompilationContext::setContinueLabel(uint32_t labelId, uint32_t blockLevel) {
	continueStmtJumpDestLabel = labelId;
	continueStmtBlockLevel = blockLevel;
}

SLKC_API uint32_t NormalCompilationContext::getBreakLabel() {
	return breakStmtJumpDestLabel;
}
SLKC_API uint32_t NormalCompilationContext::getContinueLabel() {
	return continueStmtJumpDestLabel;
}

SLKC_API uint32_t NormalCompilationContext::getBreakLabelBlockLevel() {
	return breakStmtBlockLevel;
}

SLKC_API uint32_t NormalCompilationContext::getContinueLabelBlockLevel() {
	return continueStmtBlockLevel;
}

SLKC_API uint32_t NormalCompilationContext::getCurInsOff() const {
	return baseInsOff + generatedInstructions.size();
}

SLKC_API std::optional<CompilationError> NormalCompilationContext::enterBlock() {
	if (!savedBlockLayers.pushBack(std::move(curBlockLayer))) {
		return genOutOfMemoryCompError();
	}

	curBlockLayer = BlockLayer(allocator.get());

	return {};
}
SLKC_API void NormalCompilationContext::leaveBlock() {
	curBlockLayer = std::move(savedBlockLayers.back());
	savedBlockLayers.popBack();
}

SLKC_API uint32_t NormalCompilationContext::getBlockLevel() {
	return baseBlockLevel + savedBlockLayers.size();
}

SLKC_API CompileEnvironment::~CompileEnvironment() {
}

SLKC_API void CompileEnvironment::onRefZero() noexcept {
	peff::destroyAndRelease<CompileEnvironment>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLKC_API std::optional<CompilationError> slkc::evalExprType(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	const AstNodePtr<ExprNode> &expr,
	AstNodePtr<TypeNameNode> &typeOut,
	AstNodePtr<TypeNameNode> desiredType) {
	NormalCompilationContext tmpContext(compileEnv, compilationContext);

	CompileExprResult result(compileEnv->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, &tmpContext, expr, ExprEvalPurpose::EvalType, desiredType, UINT32_MAX, result));

	typeOut = result.evaluatedType;
	return {};
}

SLKC_API std::optional<CompilationError> slkc::completeParentModules(
	CompileEnvironment *compileEnv,
	IdRef *modulePath,
	AstNodePtr<ModuleNode> leaf) {
	peff::DynArray<AstNodePtr<ModuleNode>> modules(compileEnv->allocator.get());
	size_t idxNewModulesBegin = 0;

	if (!modules.resize(modulePath->entries.size())) {
		return genOutOfMemoryCompError();
	}

	AstNodePtr<ModuleNode> node = compileEnv->document->rootModule;

	for (size_t i = 0; i < modules.size(); ++i) {
		if (auto it = node->memberIndices.find(modulePath->entries.at(i).name); it != node->memberIndices.end()) {
			node = node->members.at(it.value()).template castTo<ModuleNode>();
			modules.at(i) = node;
			idxNewModulesBegin = i + 1;
		} else {
			if (i + 1 == modules.size()) {
				node = leaf;
			} else {
				if (!(node = makeAstNode<ModuleNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}
			}
			modules.at(i) = node;
			if (!node->name.build(modulePath->entries.at(i).name)) {
				return genOutOfMemoryCompError();
			}
		}
	}

	if (!leaf->name.build(modulePath->entries.back().name)) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = idxNewModulesBegin; i < modules.size(); ++i) {
		auto &currentEntry = modulePath->entries.at(i);

		if (i) {
			auto m1 = modules.at(i - 1), m2 = modules.at(i);
			if (!modules.at(i - 1)->addMember(modules.at(i).template castTo<MemberNode>())) {
				return genOutOfMemoryCompError();
			}
			modules.at(i)->setParent(modules.at(i - 1).get());
		} else {
			if (!compileEnv->document->rootModule->addMember(modules.at(i).template castTo<MemberNode>())) {
				return genOutOfMemoryCompError();
			}
			modules.at(i)->setParent(compileEnv->document->rootModule);
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::cleanupUnusedModuleTree(
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> leaf) {
	AstNodePtr<ModuleNode> cur = leaf;

	for (;;) {
		for (auto &i : cur->members) {
			if (i->astNodeType == AstNodeType::Module) {
				return {};
			}
		}

		if (!cur->parent) {
			break;
		}

		if (cur->parent->astNodeType != AstNodeType::Module)
			std::terminate();

		AstNodePtr<ModuleNode> parent = cur->parent->sharedFromThis().template castTo<ModuleNode>();

		if (!parent->removeMember(cur->name))
			return genOutOfMemoryCompError();

		cur = parent;
	}

	return {};
}

ExternalModuleProvider::ExternalModuleProvider(const char *providerName) : providerName(providerName) {
}

ExternalModuleProvider::~ExternalModuleProvider() {
}

FileSystemExternalModuleProvider::FileSystemExternalModuleProvider(peff::Alloc *allocator) : ExternalModuleProvider("filesystem"), importPaths(allocator) {
}

FileSystemExternalModuleProvider::~FileSystemExternalModuleProvider() {
}

SLKC_API std::optional<CompilationError> FileSystemExternalModuleProvider::loadModule(CompileEnvironment *compileEnv, IdRef *moduleName) {
	peff::String suffixPath(compileEnv->allocator.get());

	{
		bool isLoaded = true;
		AstNodePtr<ModuleNode> node = compileEnv->document->rootModule;

		for (size_t i = 0; i < moduleName->entries.size(); ++i) {
			if (auto it = node->memberIndices.find(moduleName->entries.at(i).name); it != node->memberIndices.end()) {
				node = node->members.at(it.value()).template castTo<ModuleNode>();
				continue;
			}

			isLoaded = false;
			break;
		}

		if (isLoaded) {
			return {};
		}
	}

	for (size_t i = 0; i < moduleName->entries.size(); ++i) {
		auto &currentEntry = moduleName->entries.at(i);

		if (currentEntry.genericArgs.size()) {
			return CompilationError(moduleName->tokenRange, CompilationErrorKind::MalformedModuleName);
		}

		size_t beginIndex = suffixPath.size();

		if (!suffixPath.resize(beginIndex + sizeof('/') + currentEntry.name.size())) {
			return genOutOfMemoryCompError();
		}

		suffixPath.at(beginIndex) = '/';

		memcpy(suffixPath.data() + beginIndex + 1, currentEntry.name.data(), currentEntry.name.size());
	}

	for (size_t i = 0; i < importPaths.size(); ++i) {
		const peff::String &curPath = importPaths.at(i);

		{
			peff::String fullPath(compileEnv->allocator.get());

			const static char extension[] = ".slk";

			if (!fullPath.resize(curPath.size() + suffixPath.size() + strlen(extension))) {
				return genOutOfMemoryCompError();
			}

			memcpy(fullPath.data(), curPath.data(), curPath.size());
			memcpy(fullPath.data() + curPath.size(), suffixPath.data(), suffixPath.size());
			memcpy(fullPath.data() + curPath.size() + suffixPath.size(), extension, strlen(extension));

			FILE *fp = fopen(fullPath.data(), "rb");
			if (fp) {
				peff::ScopeGuard closeFpGuard([fp]() noexcept {
					if (fp) {
						fclose(fp);
					}
				});

				fseek(fp, 0, SEEK_END);
				long fileSize = ftell(fp);
				if (fileSize < 0) {
					goto fail;
				}
				fseek(fp, 0, SEEK_SET);

				auto deleter = [compileEnv, fileSize](void *ptr) {
					if (ptr) {
						compileEnv->allocator->release(ptr, (size_t)fileSize, 1);
					}
				};
				std::unique_ptr<char, decltype(deleter)> fileContent((char *)malloc((size_t)fileSize + 1), std::move(deleter));
				if (!fileContent) {
					goto fail;
				}

				if (fread(fileContent.get(), (size_t)fileSize, 1, fp) < 1) {
					goto fail;
				}

				AstNodePtr<ModuleNode> mod;

				if (!(mod = makeAstNode<ModuleNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				peff::Uninitialized<slkc::TokenList> tokenList;
				{
					slkc::Lexer lexer(compileEnv->allocator.get());

					std::string_view sv(fileContent.get(), fileSize);

					if (auto e = lexer.lex(sv, peff::getDefaultAlloc(), compileEnv->document); e) {
						auto ce = CompilationError(moduleName->tokenRange, ErrorParsingImportedModuleErrorExData(std::move(*e)));
						e.reset();
						return std::move(ce);
					}

					tokenList.moveFrom(std::move(lexer.tokenList));
				}

				peff::SharedPtr<slkc::Parser> parser;
				if (!(parser = peff::makeShared<slkc::Parser>(compileEnv->allocator.get(), compileEnv->document, tokenList.release(), compileEnv->allocator.get()))) {
					return genOutOfMemoryCompError();
				}

				IdRefPtr moduleName;
				if (auto e = parser->parseProgram(mod, moduleName); e) {
					if (!parser->syntaxErrors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(completeParentModules(compileEnv, moduleName.get(), mod));

				if (parser->syntaxErrors.size()) {
					return CompilationError(moduleName->tokenRange, ErrorParsingImportedModuleErrorExData(mod));
				}

				for (auto i : mod->members) {
					if (i->astNodeType == AstNodeType::Import) {
						SLKC_RETURN_IF_COMP_ERROR(loadModule(compileEnv, i.template castTo<ImportNode>()->idRef.get()));
					}
				}
				for (auto i : mod->anonymousImports) {
					SLKC_RETURN_IF_COMP_ERROR(loadModule(compileEnv, i->idRef.get()));
				}

				return {};
			}
		fail:;
		}

		{
			peff::String fullPath(compileEnv->allocator.get());

			const static char extension[] = ".slx";

			if (!fullPath.resize(curPath.size() + suffixPath.size() + strlen(extension))) {
				return genOutOfMemoryCompError();
			}

			memcpy(fullPath.data(), curPath.data(), curPath.size());
			memcpy(fullPath.data() + curPath.size(), suffixPath.data(), suffixPath.size());
			memcpy(fullPath.data() + curPath.size() + suffixPath.size(), extension, strlen(extension));

			FILE *fp = fopen(fullPath.data(), "rb");
			if (fp) {
				peff::ScopeGuard closeFpGuard([fp]() noexcept {
					if (fp) {
						fclose(fp);
					}
				});

				fseek(fp, 0, SEEK_END);
				long fileSize = ftell(fp);
				if (fileSize < 0) {
					goto moduleFail;
				}
				fseek(fp, 0, SEEK_SET);

				auto deleter = [compileEnv, fileSize](void *ptr) {
					if (ptr) {
						compileEnv->allocator->release(ptr, (size_t)fileSize, 1);
					}
				};
				std::unique_ptr<char, decltype(deleter)> fileContent((char *)malloc((size_t)fileSize), std::move(deleter));
				if (!fileContent) {
					goto moduleFail;
				}

				if (fread(fileContent.get(), (size_t)fileSize, 1, fp) < 1) {
					goto moduleFail;
				}

				/* TODO: Implement it.*/
			}
		moduleFail:;
		}
	}

	return CompilationError(moduleName->tokenRange, CompilationErrorKind::ModuleNotFound);
}

SLKC_API bool FileSystemExternalModuleProvider::registerImportPath(peff::String &&path) {
	if (!importPaths.pushBack(std::move(path))) {
		return false;
	}
	return true;
}
