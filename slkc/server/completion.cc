#include "server.h"
#include "../compiler/compiler.h"
#include <filesystem>

using namespace slake::slkc;

static std::string keywordConditions[] = {
	"async",
	"await",
	"base",
	"break",
	"case",
	"catch",
	"class",
	"const",
	"continue",
	"delete",
	"default",
	"else",
	"enum",
	"false",
	"fn",
	"for",
	"final",
	"if",
	"module",
	"native",
	"new",
	"null",
	"override",
	"operator",
	"pub",
	"return",
	"static",
	"struct",
	"switch",
	"this",
	"throw",
	"interface",
	"true",
	"try",
	"use",
	"while",
	"yield"
};

static std::string typeNameConditions[] = {
	"i8",
	"i16",
	"i32",
	"i64",
	"u8",
	"u16",
	"u32",
	"u64",
	"f32",
	"f64",
	"std::string",
	"bool",
	"auto",
	"void",
	"any"
};

void slake::slkc::_walkForCompletion(
	SourceDocument *document,
	Scope *scope,
	std::unordered_map<std::string, MemberNode *> &membersOut,
	std::set<Scope *> &walkedScopes,
	bool isTopLevelRef,
	bool isStatic) {
	if (walkedScopes.count(scope))
		return;
	walkedScopes.insert(scope);

	for (auto &i : scope->members) {
		if (!membersOut.count(i.first)) {
			switch (i.second->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface:
				case NodeType::Module:
					if (isStatic)
						membersOut[i.first] = i.second.get();
					break;
				case NodeType::Var: {
					auto m = std::static_pointer_cast<VarNode>(i.second);

					if (isStatic) {
						if (m->access & ACCESS_STATIC) {
							membersOut[i.first] = m.get();
						}
					} else {
						if (!(m->access & ACCESS_STATIC)) {
							membersOut[i.first] = m.get();
						}
					}
					break;
				}
				case NodeType::Fn: {
					auto m = std::static_pointer_cast<FnNode>(i.second);

					for (auto &j : m->overloadingRegistries) {
						if (isStatic) {
							if (j->access & ACCESS_STATIC) {
								membersOut[i.first] = m.get();
								break;
							}
						} else {
							if (!(j->access & ACCESS_STATIC)) {
								membersOut[i.first] = m.get();
								break;
							}
						}
					}
					break;
				}
			}
		}
	}

	if (scope->owner)
		_walkForCompletion(document, scope->owner, membersOut, walkedScopes, isStatic);

	if (isTopLevelRef) {
		if (scope->parent)
			_walkForCompletion(document, scope->parent, membersOut, walkedScopes, isTopLevelRef, isStatic);
	}
}

void slake::slkc::_walkForCompletion(
	SourceDocument *document,
	AstNode *m,
	std::unordered_map<std::string, MemberNode *> &membersOut,
	std::set<Scope *> &walkedScopes,
	bool isStatic) {
	switch (m->getNodeType()) {
		case NodeType::Class: {
			ClassNode *node = (ClassNode *)m;

			_walkForCompletion(document, node->scope.get(), membersOut, walkedScopes, false, isStatic);

			if (node->parentClass) {
				auto parent = document->compiler->resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());
				_walkForCompletion(document, parent.get(), membersOut, walkedScopes, isStatic);
			}

			for (auto &i : node->implInterfaces) {
				auto parent = document->compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(document, parent.get(), membersOut, walkedScopes, isStatic);
			}
			break;
		}
		case NodeType::Interface: {
			InterfaceNode *node = (InterfaceNode *)m;

			for (auto &i : node->parentInterfaces) {
				auto parent = document->compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(document, parent.get(), membersOut, walkedScopes, isStatic);
			}
			break;
		}
	}
}

std::unordered_map<std::string, MemberNode *> slake::slkc::_walkForCompletion(
	SourceDocument *document,
	Scope *scope,
	bool isTopLevelRef,
	bool isStatic) {
	std::unordered_map<std::string, MemberNode *> membersOut;
	std::set<Scope *> walkedScopes;

	_walkForCompletion(document, scope, membersOut, walkedScopes, isTopLevelRef, isStatic);
	if (scope->owner)
		_walkForCompletion(document, scope->owner, membersOut, walkedScopes, isStatic);

	return membersOut;
}

CompletionItemType slake::slkc::_toCompletionItemType(NodeType nodeType) {
	switch (nodeType) {
		case NodeType::Var:
			return CompletionItemType::Var;
		case NodeType::LocalVar:
			return CompletionItemType::LocalVar;
		case NodeType::Param:
			return CompletionItemType::Param;
		case NodeType::Fn:
			return CompletionItemType::Fn;
		case NodeType::TypeName:
			return CompletionItemType::Type;
		case NodeType::GenericParam:
			return CompletionItemType::GenericParam;
		case NodeType::Class:
			return CompletionItemType::Class;
		case NodeType::Interface:
			return CompletionItemType::Interface;
		case NodeType::Module:
			return CompletionItemType::Module;
			/*case NodeType::Enum:
				return CompletionItemType::Enum;
			case NodeType::EnumConst:
				return CompletionItemType::EnumConst;*/
	}

	throw std::logic_error("Unrecognized node type");
}

void slake::slkc::_getCompletionItems(
	const std::unordered_map<std::string, MemberNode *> &members,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	for (auto &i : members) {
		if (!targetNodeTypes.count(i.second->getNodeType()))
			continue;

		CompletionItem item = {};

		item.label = i.first;
		item.type = _toCompletionItemType(i.second->getNodeType());

		completionItems.push_back(item);
	}
}

std::deque<CompletionItem> slake::slkc::getCompletionItems(
	SourceDocument *document,
	SourcePosition location) {
	std::deque<CompletionItem> completionItems;

	size_t idxToken = document->lexer->getTokenByPosition(location);

	if (idxToken != SIZE_MAX) {
		do {
			switch (document->lexer->tokens[idxToken]->tokenId) {
				case TokenId::Whitespace:
				case TokenId::NewLine:
				case TokenId::LineComment:
				case TokenId::BlockComment:
				case TokenId::DocumentationComment:
					break;
				default:
					goto succeeded;
			}
		} while (idxToken--);
	}

	_getCompletionItems(
		_walkForCompletion(document, document->compiler->_rootNode->scope.get(), true, true),
		completionItems,
		{ NodeType::GenericParam,
			NodeType::Class,
			NodeType::Interface,
			NodeType::Module });

	return completionItems;

succeeded:
	Token *token = document->lexer->tokens[idxToken].get();
	TokenInfo &tokenInfo = document->tokenInfos[idxToken];

	if (tokenInfo.semanticInfo.correspondingParam) {
		CompletionItem item = {};

		item.label = "*";

		completionItems.push_back(item);
	}

	/*
	for (size_t i = 0; i < keywordConditions->size(); ++i) {
		std::string &s = keywordConditions[i];
		if (s.find(token.text) != std::string::npos)
			completionItems.push_back({ CompletionItemType::Keyword, s, s + " keyword", "", false });
	}

	for (size_t i = 0; i < typeNameConditions->size(); ++i) {
		std::string &s = typeNameConditions[i];
		if (s.find(token.text) != std::string::npos)
			completionItems.push_back({ CompletionItemType::Type, s, s + " type name", "", false });
	}
	*/

	switch (tokenInfo.completionContext) {
		case CompletionContext::TopLevel: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(document, tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });

			break;
		}
		case CompletionContext::Class: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(document, tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });
			break;
		}
		case CompletionContext::Interface: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(document, tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });
			break;
		}
		case CompletionContext::Stmt: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(
						document,
						tokenInfo.tokenContext.curScope.get(),
						tokenInfo.semanticInfo.isTopLevelRef,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });

			if (tokenInfo.semanticInfo.isTopLevelRef) {
				for (auto &i : tokenInfo.tokenContext.localVars) {
					CompletionItem item = {};

					item.label = i.first;
					item.type = CompletionItemType::LocalVar;

					completionItems.push_back(item);
				}

				for (auto &i : tokenInfo.tokenContext.paramIndices) {
					CompletionItem item = {};

					item.label = i.first;
					item.type = CompletionItemType::GenericParam;

					completionItems.push_back(item);
				}
			}
			break;
		}
		case CompletionContext::Import: {
			std::string path;

			for (size_t i = 0; i < tokenInfo.semanticInfo.importedPath->entries.size(); ++i) {
				if (i)
					path += "/";
				path += tokenInfo.semanticInfo.importedPath->entries[i].name;
			}

			_getImportCompletionItems(document->compiler, path, completionItems);
			break;
		}
		case CompletionContext::ModuleName: {
			std::string path;

			for (size_t i = 0; i < tokenInfo.semanticInfo.importedPath->entries.size(); ++i) {
				if (i)
					path += "/";
				path += tokenInfo.semanticInfo.importedPath->entries[i].name;
			}

			_getImportCompletionItems(document->compiler, path, completionItems);
			break;
		}
		case CompletionContext::Type: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(
						document,
						tokenInfo.tokenContext.curScope.get(),
						tokenInfo.semanticInfo.isTopLevelRef,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });

			for (auto &i : tokenInfo.tokenContext.genericParams) {
				CompletionItem item = {};

				item.label = i->name;
				item.type = CompletionItemType::GenericParam;

				completionItems.push_back(item);
			}
			break;
		}
		case CompletionContext::Name:
			return {};
		case CompletionContext::Expr: {
			if (tokenInfo.tokenContext.curScope) {
				// A token has `expr` type means it is the top level scope of
				// a reference, which means we can resolve it with its parent
				// scope.
				_getCompletionItems(
					_walkForCompletion(
						document,
						tokenInfo.tokenContext.curScope.get(),
						tokenInfo.semanticInfo.isTopLevelRef,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });
			}

			if (tokenInfo.semanticInfo.isTopLevelRef) {
				for (auto &i : tokenInfo.tokenContext.localVars) {
					CompletionItem item = {};

					item.label = i.first;
					item.type = CompletionItemType::LocalVar;

					completionItems.push_back(item);
				}

				for (auto &i : tokenInfo.tokenContext.paramIndices) {
					CompletionItem item = {};

					item.label = i.first;
					item.type = CompletionItemType::GenericParam;

					completionItems.push_back(item);
				}
			}
			break;
		}
		case CompletionContext::MemberAccess: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(
						document,
						tokenInfo.tokenContext.curScope.get(),
						false,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Module });
			break;
		}
		case CompletionContext::None:
			break;
		default:
			assert(false);
	}

	return completionItems;
}

void slake::slkc::_getImportCompletionItems(Compiler *compiler, std::string path, std::deque<CompletionItem> &completionItems) {
	std::set<std::string> foundModuleNames;

	for (auto i : compiler->modulePaths) {
#ifdef _WIN32
		std::string contactedPath = i + "\\" + path;

		std::string findPath;

		WIN32_FIND_DATAA findData;
		HANDLE hFindFile;

		findData = {};
		findPath = contactedPath + "\\*.slk";

		hFindFile = FindFirstFileA(findPath.c_str(), &findData);
		do {
			if (hFindFile == INVALID_HANDLE_VALUE)
				break;

			std::string fileName = findData.cFileName;

			size_t idxLastPathSeparator = fileName.find_last_of('/');
			if (idxLastPathSeparator != std::string::npos)
				fileName = fileName.substr(idxLastPathSeparator);

			fileName = fileName.substr(0, fileName.size() - 4);

			if (!foundModuleNames.count(fileName)) {
				CompletionItem item = {};
				item.label = fileName;
				item.type = CompletionItemType::Module;

				completionItems.push_back(item);

				foundModuleNames.insert(fileName);
			}
		} while (FindNextFileA(hFindFile, &findData));

		findData = {};
		findPath = contactedPath + "\\*.slx";

		hFindFile = FindFirstFileA(findPath.c_str(), &findData);
		do {
			if (hFindFile == INVALID_HANDLE_VALUE)
				break;

			std::string fileName = findData.cFileName;

			size_t idxLastPathSeparator = fileName.find_last_of('/');
			if (idxLastPathSeparator != std::string::npos)
				fileName = fileName.substr(idxLastPathSeparator);

			fileName = fileName.substr(0, fileName.size() - 4);

			if (!foundModuleNames.count(fileName)) {
				CompletionItem item = {};
				item.label = fileName;
				item.type = CompletionItemType::Module;

				completionItems.push_back(item);

				foundModuleNames.insert(fileName);
			}
		} while (FindNextFileA(hFindFile, &findData));

		findData = {};
		findPath = contactedPath + "\\*.*";

		hFindFile = FindFirstFileA(findPath.c_str(), &findData);
		do {
			if (hFindFile == INVALID_HANDLE_VALUE)
				break;

			std::string fileName = findData.cFileName;

			if (fileName == "." || fileName == "..")
				continue;

			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;

			size_t idxLastPathSeparator = fileName.find_last_of('/');
			if (idxLastPathSeparator != std::string::npos)
				fileName = fileName.substr(idxLastPathSeparator);

			if (!foundModuleNames.count(fileName)) {
				CompletionItem item = {};
				item.label = fileName;
				item.type = CompletionItemType::Module;

				completionItems.push_back(item);

				foundModuleNames.insert(fileName);
			}
		} while (FindNextFileA(hFindFile, &findData));
#else
#endif
	}
}
