#include "server.h"
#include "../compiler/compiler.h"

using namespace slake::slkc;

static string keywordConditions[] = {
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

static string typeNameConditions[] = {
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
	"string",
	"bool",
	"auto",
	"void",
	"any"
};

void slake::slkc::Document::_walkForCompletion(
	Scope *scope,
	std::unordered_map<std::string, MemberNode *> &membersOut,
	std::set<Scope *> &walkedScopes) {
	if (walkedScopes.count(scope))
		return;
	walkedScopes.insert(scope);

	for (auto &i : scope->members) {
		if (!membersOut.count(i.first))
			membersOut[i.first] = i.second.get();
	}

	if (scope->owner)
		_walkForCompletion(scope->owner, membersOut, walkedScopes);
}

void slake::slkc::Document::_walkForCompletion(
	AstNode *m,
	std::unordered_map<std::string, MemberNode *> &membersOut,
	std::set<Scope *> &walkedScopes) {
	switch (m->getNodeType()) {
		case NodeType::Class: {
			ClassNode *node = (ClassNode *)m;

			_walkForCompletion(node->scope.get(), membersOut, walkedScopes);

			if (node->parentClass) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)node->parentClass.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes);
			}

			for (auto &i : node->implInterfaces) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes);
			}
			break;
		}
		case NodeType::Interface: {
			InterfaceNode *node = (InterfaceNode *)m;

			for (auto &i : node->parentInterfaces) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes);
			}
			break;
		}
		case NodeType::Trait: {
			TraitNode *node = (TraitNode *)m;

			for (auto &i : node->parentTraits) {
				auto parent = compiler->resolveCustomTypeName((CustomTypeNameNode *)i.get());
				_walkForCompletion(parent.get(), membersOut, walkedScopes);
			}
			break;
		}
	}
}

std::unordered_map<std::string, MemberNode *> slake::slkc::Document::_walkForCompletion(Scope *scope, bool isTopLevelRef) {
	std::unordered_map<std::string, MemberNode *> membersOut;
	std::set<Scope *> walkedScopes;

	_walkForCompletion(scope, membersOut, walkedScopes);
	if (scope->owner)
		_walkForCompletion(scope->owner, membersOut, walkedScopes);

	if (isTopLevelRef) {
		if (scope->parent)
			_walkForCompletion(scope->parent, membersOut, walkedScopes);
	}

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
		default:
			assert(false);
	}
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

std::deque<CompletionItem> slake::slkc::Document::getCompletionItems(Location location) {
	std::deque<CompletionItem> completionItems;

	size_t idxToken = compiler->lexer.getTokenByLocation(location);

	if (idxToken == SIZE_MAX) {
		_getCompletionItems(
			_walkForCompletion(compiler->_rootScope.get(), true),
			completionItems,
			{ NodeType::GenericParam,
				NodeType::Class,
				NodeType::Interface,
				NodeType::Trait,
				NodeType::Module });
	} else {
		Token &token = compiler->lexer.tokens[idxToken];
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
						_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true),
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
						_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true),
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
						_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true),
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
						_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true),
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
							tokenInfo.semanticInfo.isTopLevelRef),
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
				if (tokenInfo.tokenContext.curScope)
					_getCompletionItems(
						_walkForCompletion(tokenInfo.tokenContext.curScope.get(), true),
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
			case CompletionContext::Type: {
				if (tokenInfo.tokenContext.curScope)
					_getCompletionItems(
						_walkForCompletion(
							tokenInfo.tokenContext.curScope.get(),
							tokenInfo.semanticInfo.isTopLevelRef),
						completionItems,
						{ NodeType::GenericParam,
							NodeType::Class,
							NodeType::Interface,
							NodeType::Trait,
							NodeType::Module });
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
							tokenInfo.semanticInfo.isTopLevelRef),
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
						_walkForCompletion(tokenInfo.tokenContext.curScope.get(), false),
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
	}

	return completionItems;
}
