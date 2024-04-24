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

void slake::slkc::Document::getCompletionMemberItems(
	Scope *scope,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	for (auto &i : scope->members) {
		CompletionItem item = {};

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

		completionItems.push_back(item);
	}
}

void slake::slkc::Document::getCompletionMemberItems(
	shared_ptr<TypeNameNode> t,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	switch (t->getTypeId()) {
		case Type::Custom: {
			shared_ptr<CustomTypeNameNode> tn = static_pointer_cast<CustomTypeNameNode>(t);

			getCompletionMemberItems(compiler->resolveCustomTypeName(tn.get()).get(), completionItems, targetNodeTypes);
			break;
		}
	}
}

void slake::slkc::Document::getCompletionMemberItems(
	AstNode *m,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	switch (m->getNodeType()) {
		case NodeType::Var: {
			VarNode *node = (VarNode *)m;

			getCompletionMemberItems(node->type, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::LocalVar: {
			LocalVarNode *node = (LocalVarNode *)m;

			getCompletionMemberItems(node->type, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Class: {
			ClassNode *node = (ClassNode *)m;
			getCompletionMemberItems(node->scope.get(), completionItems, targetNodeTypes);

			if (node->parentClass)
				getCompletionMemberItems(node->parentClass, completionItems, targetNodeTypes);

			for (auto i : node->implInterfaces)
				getCompletionMemberItems(i, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Interface: {
			InterfaceNode *node = (InterfaceNode *)m;
			getCompletionMemberItems(node->scope.get(), completionItems, targetNodeTypes);

			for (auto i : node->parentInterfaces)
				getCompletionMemberItems(i, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Trait: {
			TraitNode *node = (TraitNode *)m;
			getCompletionMemberItems(node->scope.get(), completionItems, targetNodeTypes);

			for (auto i : node->parentTraits)
				getCompletionMemberItems(i, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Module: {
			ModuleNode *node = (ModuleNode *)m;
			getCompletionMemberItems(node->scope.get(), completionItems, targetNodeTypes);

			if (!node->parentModule.expired())
				getCompletionMemberItems(node->parentModule.lock().get(), completionItems, targetNodeTypes);
			break;
		}
	}
}

std::deque<CompletionItem> slake::slkc::Document::getCompletionItems(Location location) {
	std::deque<CompletionItem> completionItems;

	size_t idxToken = compiler->lexer.getTokenByLocation(location);

	if (idxToken == SIZE_MAX)
		return {};

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

	if (auto cm = tokenInfo.tokenContext.curScope; cm) {
		getCompletionMemberItems(
			cm.get(),
			completionItems,
			{ NodeType::Var,
				NodeType::Fn,
				NodeType::GenericParam,
				NodeType::Class,
				NodeType::Interface,
				NodeType::Trait,
				NodeType::Module });

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
	} else {
		switch (tokenInfo.completionContext) {
			case CompletionContext::TopLevel: {
				if (tokenInfo.tokenContext.curScope)
					getCompletionMemberItems(
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
					getCompletionMemberItems(
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
					getCompletionMemberItems(
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
					getCompletionMemberItems(
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
					getCompletionMemberItems(
						tokenInfo.tokenContext.curScope.get(),
						completionItems,
						{ NodeType::Var,
							NodeType::Fn,
							NodeType::GenericParam,
							NodeType::Class,
							NodeType::Interface,
							NodeType::Trait,
							NodeType::Module });

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
				break;
			}
			case CompletionContext::Import: {
				if (tokenInfo.tokenContext.curScope)
					getCompletionMemberItems(
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
					getCompletionMemberItems(
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
					getCompletionMemberItems(
						tokenInfo.tokenContext.curScope.get(),
						completionItems,
						{ NodeType::Var,
							NodeType::Fn,
							NodeType::GenericParam,
							NodeType::Class,
							NodeType::Interface,
							NodeType::Trait,
							NodeType::Module });

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
				break;
			}
			case CompletionContext::MemberAccess: {
				if (tokenInfo.tokenContext.curScope)
					getCompletionMemberItems(
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
			case CompletionContext::None:
				break;
			default:
				assert(false);
		}
	}

	return completionItems;
}
