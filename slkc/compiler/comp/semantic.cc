#include "../compiler.h"

using namespace slake::slkc;

#if SLKC_WITH_LANGUAGE_SERVER

void Compiler::updateCompletionContext(size_t idxToken, CompletionContext completionContext) {
	auto curDoc = sourceDocs.at(curDocName).get();

	curDoc->tokenInfos[idxToken].completionContext = completionContext;
}

void Compiler::updateCompletionContextForTrailingSpaces(CompileContext *compileContext, size_t idxBegin, size_t idxEnd, CompletionContext completionContext) {
	assert(idxEnd >= idxBegin);

	auto curDoc = compileContext->compiler->sourceDocs.at(curDocName).get();

	for (size_t i = idxBegin; i < idxEnd; ++i) {
		switch (curDoc->lexer->tokens[i]->tokenId) {
		case TokenId::Whitespace:
		case TokenId::NewLine:
			updateCompletionContext(i, completionContext);
		case TokenId::BlockComment:
		case TokenId::LineComment:
			break;
		default:
			goto end;
		}
	}
end:;
}

void Compiler::updateCompletionContext(std::shared_ptr<TypeNameNode> targetTypeName, CompletionContext completionContext) {
	auto curDoc = sourceDocs.at(curDocName).get();

	switch (targetTypeName->getTypeId()) {
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
	case TypeId::Void:
	case TypeId::Any:
	case TypeId::Auto: {
		auto t = std::static_pointer_cast<BasicSimpleTypeNameNode>(targetTypeName);

		if (t->idxToken != SIZE_MAX)
			curDoc->tokenInfos[t->idxToken].completionContext = completionContext;
		break;
	}
	case TypeId::Array: {
		auto t = std::static_pointer_cast<ArrayTypeNameNode>(targetTypeName);
		updateCompletionContext(t->elementType, completionContext);
		break;
	}
	case TypeId::Custom: {
		auto t = std::static_pointer_cast<CustomTypeNameNode>(targetTypeName);

		updateCompletionContext(t->ref, completionContext);
		break;
	}
	case TypeId::Bad: {
		auto t = std::static_pointer_cast<BadTypeNameNode>(targetTypeName);

		for (size_t i = t->idxStartToken; i < t->idxEndToken; ++i)
			curDoc->tokenInfos[i].completionContext = completionContext;

		break;
	}
	default:
		assert(false);
	}
}

void Compiler::updateCompletionContext(std::shared_ptr<IdRefNode> ref, CompletionContext completionContext) {
	auto curDoc = sourceDocs.at(curDocName).get();

	for (size_t i = 0; i < ref->entries.size(); ++i) {
		if (ref->entries[i].idxAccessOpToken != SIZE_MAX) {
			curDoc->tokenInfos[ref->entries[i].idxAccessOpToken].completionContext = completionContext;
		}
		if (ref->entries[i].idxToken != SIZE_MAX) {
			curDoc->tokenInfos[ref->entries[i].idxToken].completionContext = completionContext;
		}
	}
}

void Compiler::updateSemanticType(size_t idxToken, SemanticType type) {
	auto curDoc = sourceDocs.at(curDocName).get();

	curDoc->tokenInfos[idxToken].semanticType = type;
}

void Compiler::updateSemanticType(std::shared_ptr<TypeNameNode> targetTypeName, SemanticType type) {
	auto curDoc = sourceDocs.at(curDocName).get();

	switch (targetTypeName->getTypeId()) {
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
	case TypeId::Void:
	case TypeId::Any:
	case TypeId::Auto: {
		auto t = std::static_pointer_cast<BasicSimpleTypeNameNode>(targetTypeName);
		if (t->idxToken != SIZE_MAX)
			curDoc->tokenInfos[t->idxToken].semanticType = type;
		break;
	}
	case TypeId::Array: {
		auto t = std::static_pointer_cast<ArrayTypeNameNode>(targetTypeName);
		updateSemanticType(t->elementType, type);
		break;
	}
	case TypeId::Custom: {
		auto t = std::static_pointer_cast<CustomTypeNameNode>(targetTypeName);

		updateSemanticType(t->ref, type);
		break;
	}
	case TypeId::Bad: {
		auto t = std::static_pointer_cast<BadTypeNameNode>(targetTypeName);

		for (size_t i = t->idxStartToken; i < t->idxEndToken; ++i)
			curDoc->tokenInfos[i].semanticType = type;

		break;
	}
	default:
		assert(false);
	}
}

void Compiler::updateSemanticType(std::shared_ptr<IdRefNode> ref, SemanticType type) {
	auto curDoc = sourceDocs.at(curDocName).get();

	for (size_t i = 0; i < ref->entries.size(); ++i) {
		if (ref->entries[i].idxAccessOpToken != SIZE_MAX) {
			curDoc->tokenInfos[ref->entries[i].idxAccessOpToken].semanticType = type;
		}
		if (ref->entries[i].idxToken != SIZE_MAX) {
			curDoc->tokenInfos[ref->entries[i].idxToken].semanticType = type;
		}
	}
}

void Compiler::updateTokenInfo(size_t idxToken, std::function<void(TokenInfo &info)> updater) {
	auto curDoc = sourceDocs.at(curDocName).get();

	if (idxToken == SIZE_MAX)
		return;

	updater(curDoc->tokenInfos[idxToken]);
}

void Compiler::updateTokenInfoForTrailingSpaces(CompileContext *compileContext, size_t idxBegin, size_t idxEnd, std::function<void(TokenInfo &info)> updater) {
	assert(idxEnd >= idxBegin);

	auto curDoc = compileContext->compiler->sourceDocs.at(curDocName).get();

	for (size_t i = idxBegin; i < idxEnd; ++i) {
		switch (curDoc->lexer->tokens[i]->tokenId) {
		case TokenId::Whitespace:
		case TokenId::NewLine:
			updateTokenInfo(i, updater);
			break;
		case TokenId::BlockComment:
		case TokenId::LineComment:
			break;
		default:
			goto end;
		}
	}
end:;
}

#endif
