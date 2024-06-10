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
	"trait",
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

void slake::slkc::Document::_walkForCompletion(
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
		_walkForCompletion(scope->owner, membersOut, walkedScopes, isStatic);

	if (isTopLevelRef) {
		if (scope->parent)
			_walkForCompletion(scope->parent, membersOut, walkedScopes, isTopLevelRef, isStatic);
	}
}

void slake::slkc::Document::_walkForCompletion(
	AstNode *m,
	std::unordered_map<std::string, MemberNode *> &membersOut,
	std::set<Scope *> &walkedScopes,
	bool isStatic) {
	switch (m->getNodeType()) {
		case NodeType::Class: {
			ClassNode *node = (ClassNode *)m;

			_walkForCompletion(node->scope.get(), membersOut, walkedScopes, false, isStatic);

			if (node->parentClass) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes, isStatic);
			}

			for (auto &i : node->implInterfaces) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes, isStatic);
			}
			break;
		}
		case NodeType::Interface: {
			InterfaceNode *node = (InterfaceNode *)m;

			for (auto &i : node->parentInterfaces) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes, isStatic);
			}
			break;
		}
		case NodeType::Trait: {
			TraitNode *node = (TraitNode *)m;

			for (auto &i : node->parentTraits) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes, isStatic);
			}
			break;
		}
	}
}

std::unordered_map<std::string, MemberNode *> slake::slkc::Document::_walkForCompletion(Scope *scope, bool isTopLevelRef, bool isStatic) {
	std::unordered_map<std::string, MemberNode *> membersOut;
	std::set<Scope *> walkedScopes;

	_walkForCompletion(scope, membersOut, walkedScopes, isTopLevelRef, isStatic);
	if (scope->owner)
		_walkForCompletion(scope->owner, membersOut, walkedScopes, isStatic);

	return membersOut;
}

CompletionItemType slake::slkc::Document::_toCompletionItemType(NodeType nodeType) {
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
		case NodeType::Trait:
			return CompletionItemType::Trait;
		case NodeType::Module:
			return CompletionItemType::Module;
			/*case NodeType::Enum:
				return CompletionItemType::Enum;
			case NodeType::EnumConst:
				return CompletionItemType::EnumConst;*/
	}

	throw std::logic_error("Unrecognized node type");
}

void slake::slkc::Document::_getCompletionItems(
	std::unordered_map<std::string, MemberNode *> &membersOut,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	for (auto &i : membersOut) {
		if (!targetNodeTypes.count(i.second->getNodeType()))
			continue;

		CompletionItem item = {};

		item.label = i.first;
		item.type = _toCompletionItemType(i.second->getNodeType());

		completionItems.push_back(item);
	}
}

std::deque<CompletionItem> slake::slkc::Document::getCompletionItems(SourcePosition location) {
	std::deque<CompletionItem> completionItems;

	size_t idxToken = compiler->lexer->getTokenByPosition(location);

	if (idxToken != SIZE_MAX) {
		do {
			switch (compiler->lexer->tokens[idxToken]->tokenId) {
				case TokenId::Whitespace:
				case TokenId::NewLine:
				case TokenId::Comment:
					break;
				default:
					goto succeeded;
			}
		} while (idxToken--);
	}

	_getCompletionItems(
		_walkForCompletion(compiler->_rootScope.get(), true, true),
		completionItems,
		{ NodeType::GenericParam,
			NodeType::Class,
			NodeType::Interface,
			NodeType::Trait,
			NodeType::Module });

	return completionItems;

succeeded:
	Token *token = compiler->lexer->tokens[idxToken].get();
	TokenInfo &tokenInfo = compiler->tokenInfos[idxToken];

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
					_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
						NodeType::Module });

			break;
		}
		case CompletionContext::Class: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
						NodeType::Module });
			break;
		}
		case CompletionContext::Interface: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
						NodeType::Module });
			break;
		}
		case CompletionContext::Trait: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true, tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
						NodeType::Module });
			break;
		}
		case CompletionContext::Stmt: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(
						tokenInfo.tokenContext.curScope.get(),
						tokenInfo.semanticInfo.isTopLevelRef,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
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

			for (size_t i = 0; i < tokenInfo.semanticInfo.importedPath.size(); ++i) {
				if (i)
					path += "/";
				path += tokenInfo.semanticInfo.importedPath[i].name;
			}

			_getImportCompletionItems(path, completionItems);
			break;
		}
		case CompletionContext::ModuleName: {
			std::string path;

			for (size_t i = 0; i < tokenInfo.semanticInfo.importedPath.size(); ++i) {
				if (i)
					path += "/";
				path += tokenInfo.semanticInfo.importedPath[i].name;
			}

			_getImportCompletionItems(path, completionItems);
			break;
		}
		case CompletionContext::Type: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					_walkForCompletion(
						tokenInfo.tokenContext.curScope.get(),
						tokenInfo.semanticInfo.isTopLevelRef,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
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
						tokenInfo.tokenContext.curScope.get(),
						tokenInfo.semanticInfo.isTopLevelRef,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
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
						tokenInfo.tokenContext.curScope.get(),
						false,
						tokenInfo.semanticInfo.isStatic),
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
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

void slake::slkc::Document::_getImportCompletionItems(std::string path, std::deque<CompletionItem> &completionItems) {
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
