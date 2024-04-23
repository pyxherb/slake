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

void slake::slkc::Document::getCompletionItems(
	Scope *scope,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	for (auto &i : scope->members) {
		CompletionItem item;

		item.label = i.first;
		switch (i.second->getNodeType()) {
			case NodeType::Var:
				if (targetNodeTypes.count(NodeType::Var)) {
					item.type = CompletionItemType::Var;
				}
				break;
			case NodeType::LocalVar:
				assert(false);
				break;
			case NodeType::Fn:
				if (targetNodeTypes.count(NodeType::Fn)) {
					item.type = CompletionItemType::Fn;
				}
				break;
			case NodeType::GenericParam:
				if (targetNodeTypes.count(NodeType::GenericParam)) {
					item.type = CompletionItemType::GenericParam;
				}
				break;
			case NodeType::Class:
				if (targetNodeTypes.count(NodeType::Class)) {
					item.type = CompletionItemType::Class;
				}
				break;
			case NodeType::Interface:
				if (targetNodeTypes.count(NodeType::Interface)) {
					item.type = CompletionItemType::Interface;
				}
				break;
			case NodeType::Trait:
				if (targetNodeTypes.count(NodeType::Trait)) {
					item.type = CompletionItemType::Trait;
				}
				break;
			case NodeType::Module:
				if (targetNodeTypes.count(NodeType::Module)) {
					item.type = CompletionItemType::Module;
				}
				break;
		}
	}

	if (scope->parent)
		getCompletionItems(scope->parent, completionItems, targetNodeTypes);
}

std::deque<CompletionItem> slake::slkc::Document::getCompletionItems(Location location) {
	std::deque<CompletionItem> completionItems;

	size_t idxToken = compiler->lexer.getTokenByLocation(location);

	if (idxToken == SIZE_MAX)
		return {};

	Token &token = compiler->lexer.tokens[idxToken];
	TokenInfo &tokenInfo = compiler->tokenInfos[idxToken];

	for (auto i : keywordConditions) {
		if (i.find(token.text) != std::string::npos)
			completionItems.push_back({ CompletionItemType::Keyword, i, i + " keyword", "", false });
	}

	for (auto i : typeNameConditions) {
		if (i.find(token.text) != std::string::npos)
			completionItems.push_back({ CompletionItemType::Type, i, i + " type name", "", false });
	}

	switch (tokenInfo.completionContext) {
		case CompletionContext::TopLevel: {
			if (tokenInfo.tokenContext.curScope)
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
		case CompletionContext::Import: {
			if (tokenInfo.tokenContext.curScope)
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
			if (tokenInfo.tokenContext.curScope)
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
		case CompletionContext::MemberAccess: {
			if (tokenInfo.tokenContext.curScope)
				getCompletionItems(
					tokenInfo.tokenContext.curScope.get(),
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
		default:
			assert(false);
	}
}
