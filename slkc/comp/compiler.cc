#include "compiler.h"

using namespace slkc;

SLKC_API CompileContext::~CompileContext() {
}

SLKC_API void CompileContext::onRefZero() noexcept {
	peff::destroyAndRelease<CompileContext>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API std::optional<CompilationError> CompileContext::emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) {
	if (flags & COMPCTXT_NOCOMPILE) {
		return {};
	}

	slake::Instruction insOut;

	insOut.opcode = opcode;
	insOut.output = outputRegIndex;
	if (operands.size()) {
		if (!(insOut.operands = (slake::Value *)allocator->alloc(sizeof(slake::Value) * operands.size(), sizeof(std::max_align_t)))) {
			return genOutOfMemoryCompError();
		}
	}
	insOut.nOperands = operands.size();
	insOut.operandsAllocator = allocator;

	auto it = operands.begin();
	for (size_t i = 0; i < operands.size(); ++i) {
		insOut.operands[i] = *it++;
	}

	if (!fnCompileContext.instructionsOut.pushBack(std::move(insOut))) {
		return genOutOfMemoryCompError();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::completeParentModules(
	CompileContext *compileContext,
	IdRef *modulePath,
	peff::SharedPtr<ModuleNode> leaf) {
	peff::DynArray<peff::SharedPtr<ModuleNode>> modules(compileContext->allocator.get());

	if (!modules.resize(modulePath->entries.size() - 1)) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < modules.size() - 1; ++i) {
		if (!(modules.at(i) = peff::makeShared<ModuleNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
			return genOutOfMemoryCompError();
		}

		if (!modules.at(i)->name.build(modulePath->entries.at(i).name)) {
			return genOutOfMemoryCompError();
		}
	}

	for (size_t i = 0; i < modulePath->entries.size() - 1; ++i) {
		auto &currentEntry = modulePath->entries.at(i);

		if (i) {
			modules.at(i)->setParent(modules.at(i - 1).get());
		} else {
			modules.at(i)->setParent(compileContext->document->rootModule);
		}
	}

	if (!leaf->name.build(modulePath->entries.back().name)) {
		return genOutOfMemoryCompError();
	}
	leaf->setParent(modules.back().get());
	return {};
}

SLKC_API std::optional<CompilationError> slkc::cleanupUnusedModuleTree(
	CompileContext *compileContext,
	peff::SharedPtr<ModuleNode> leaf) {
	peff::SharedPtr<ModuleNode> cur = leaf;

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

		peff::SharedPtr<ModuleNode> parent = cur->parent->sharedFromThis().castTo<ModuleNode>();

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

SLKC_API std::optional<CompilationError> FileSystemExternalModuleProvider::loadModule(CompileContext *compileContext, IdRef *moduleName) {
	peff::String suffixPath(compileContext->allocator.get());

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
			peff::String fullPath(compileContext->allocator.get());

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

				auto deleter = [compileContext, fileSize](void *ptr) {
					if (ptr) {
						compileContext->allocator->release(ptr, (size_t)fileSize, 1);
					}
				};
				std::unique_ptr<char, decltype(deleter)> fileContent((char *)malloc((size_t)fileSize), std::move(deleter));
				if (!fileContent) {
					goto fail;
				}

				if (fread(fileContent.get(), (size_t)fileSize, 1, fp) < 1) {
					goto fail;
				}

				peff::SharedPtr<ModuleNode> mod;

				if (!(mod = peff::makeShared<ModuleNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(completeParentModules(compileContext, moduleName, mod));

				peff::Uninitialized<slkc::TokenList> tokenList;
				{
					slkc::Lexer lexer(compileContext->allocator.get());

					std::string_view sv(fileContent.get(), fileSize);

					if (auto e = lexer.lex(sv, peff::getDefaultAlloc(), compileContext->document); e) {
						auto ce = CompilationError(moduleName->tokenRange, ErrorParsingImportedModuleErrorExData(std::move(*e)));
						e.reset();
						return std::move(ce);
					}

					tokenList.moveFrom(std::move(lexer.tokenList));
				}

				peff::SharedPtr<slkc::Parser> parser;
				if (!(parser = peff::makeShared<slkc::Parser>(compileContext->allocator.get(), compileContext->document, tokenList.release(), compileContext->allocator.get()))) {
					return genOutOfMemoryCompError();
				}

				if (auto e = parser->parseProgram(mod); e) {
					if (!parser->syntaxErrors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
				}

				if (parser->syntaxErrors.size()) {
					return CompilationError(moduleName->tokenRange, ErrorParsingImportedModuleErrorExData(mod));
				}

				return {};
			}
		fail:;
		}

		{
			peff::String fullPath(compileContext->allocator.get());

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

				auto deleter = [compileContext, fileSize](void *ptr) {
					if (ptr) {
						compileContext->allocator->release(ptr, (size_t)fileSize, 1);
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
