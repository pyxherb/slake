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
	std::unordered_map<std::string, MemberNode*>& membersOut,
	std::deque<CompletionItem>& completionItems,
	const std::set<NodeType>& targetNodeTypes) {
	for (auto& i : membersOut) {
		if (!targetNodeTypes.count(i.second->getNodeType()))
			continue;

		CompletionItem item = {};

		item.label = i.first;
		item.type = _toCompletionItemType(i.second->getNodeType());

		completionItems.push_back(item);
	}
}

void slake::slkc::Document::_getCompletionItems(
	Scope* scope,
	std::deque<CompletionItem>& completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	std::unordered_map<std::string, MemberNode *> membersOut;

	compiler->getMemberNodes(scope, membersOut);

	_getCompletionItems(membersOut, completionItems, targetNodeTypes);
}

void slake::slkc::Document::_getCompletionItems(
	shared_ptr<TypeNameNode> t,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	switch (t->getTypeId()) {
		case Type::Custom: {
			shared_ptr<CustomTypeNameNode> tn = static_pointer_cast<CustomTypeNameNode>(t);
			auto tnDest = compiler->resolveCustomTypeName(tn.get());

			_getCompletionItems(tnDest.get(), completionItems, targetNodeTypes);
			break;
		}
	}
}

void slake::slkc::Document::_getCompletionItems(
	AstNode *m,
	std::deque<CompletionItem> &completionItems,
	const std::set<NodeType> &targetNodeTypes) {
	switch (m->getNodeType()) {
		case NodeType::Var: {
			VarNode *node = (VarNode *)m;

			_getCompletionItems(node->type, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::LocalVar: {
			LocalVarNode *node = (LocalVarNode *)m;

			_getCompletionItems(node->type, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Class: {
			ClassNode *node = (ClassNode *)m;

			std::unordered_map<std::string, MemberNode *> membersOut;

			compiler->getMemberNodes(node, membersOut);

			_getCompletionItems(membersOut, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Interface: {
			InterfaceNode *node = (InterfaceNode *)m;

			std::unordered_map<std::string, MemberNode *> membersOut;

			compiler->getMemberNodes(node, membersOut);

			_getCompletionItems(membersOut, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Trait: {
			TraitNode *node = (TraitNode *)m;

			std::unordered_map<std::string, MemberNode *> membersOut;

			compiler->getMemberNodes(node, membersOut);

			_getCompletionItems(membersOut, completionItems, targetNodeTypes);
			break;
		}
		case NodeType::Module: {
			ModuleNode *node = (ModuleNode *)m;

			std::unordered_map<std::string, MemberNode *> membersOut;

			compiler->getMemberNodes(node->scope.get(), membersOut);

			_getCompletionItems(membersOut, completionItems, targetNodeTypes);
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

	switch (tokenInfo.completionContext) {
		case CompletionContext::TopLevel: {
			if (tokenInfo.tokenContext.curScope)
				_getCompletionItems(
					tokenInfo.tokenContext.curScope->owner,
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
					tokenInfo.tokenContext.curScope->owner,
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
					tokenInfo.tokenContext.curScope->owner,
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
					tokenInfo.tokenContext.curScope->owner,
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
					tokenInfo.tokenContext.curScope->owner,
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
				_getCompletionItems(
					tokenInfo.tokenContext.curScope->owner,
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
					tokenInfo.tokenContext.curScope->owner,
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
					tokenInfo.tokenContext.curScope->owner,
					completionItems,
					{ NodeType::Var,
						NodeType::Fn,
						NodeType::GenericParam,
						NodeType::Class,
						NodeType::Interface,
						NodeType::Trait,
						NodeType::Module });

				for (Scope *scope = tokenInfo.tokenContext.curScope.get(); scope; scope = scope->parent) {
					_getCompletionItems(
						scope,
						completionItems,
						{ NodeType::Var,
							NodeType::Fn,
							NodeType::GenericParam,
							NodeType::Class,
							NodeType::Interface,
							NodeType::Trait,
							NodeType::Module });
				}
			}

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
				_getCompletionItems(
					tokenInfo.tokenContext.curScope->owner,
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
