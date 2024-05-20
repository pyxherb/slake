#include "../compiler.h"

using namespace slake::slkc;

#if SLKC_WITH_LANGUAGE_SERVER

void Compiler::updateCompletionContext(size_t idxToken, CompletionContext completionContext) {
	if (curMajorContext.isImport)
		return;

	tokenInfos[idxToken].completionContext = completionContext;
}

void Compiler::updateCompletionContext(std::shared_ptr<TypeNameNode> targetTypeName, CompletionContext completionContext) {
	if (curMajorContext.isImport)
		return;

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
			assert(t->idxToken != SIZE_MAX);
			tokenInfos[t->idxToken].completionContext = completionContext;
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
				tokenInfos[i].completionContext = completionContext;

			break;
		}
		default:
			assert(false);
	}
}

void Compiler::updateCompletionContext(const IdRef &ref, CompletionContext completionContext) {
	if (curMajorContext.isImport)
		return;

	for (size_t i = 0; i < ref.size(); ++i) {
		if (ref[i].idxAccessOpToken != SIZE_MAX) {
			tokenInfos[ref[i].idxAccessOpToken].completionContext = completionContext;
		}
		if (ref[i].idxToken != SIZE_MAX) {
			tokenInfos[ref[i].idxToken].completionContext = completionContext;
		}
	}
}

void Compiler::updateSemanticType(size_t idxToken, SemanticType type) {
	if (curMajorContext.isImport)
		return;

	tokenInfos[idxToken].semanticType = type;
}

void Compiler::updateSemanticType(std::shared_ptr<TypeNameNode> targetTypeName, SemanticType type) {
	if (curMajorContext.isImport)
		return;

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
			assert(t->idxToken != SIZE_MAX);
			tokenInfos[t->idxToken].semanticType = type;
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
				tokenInfos[i].semanticType = type;

			break;
		}
		default:
			assert(false);
	}
}

void Compiler::updateSemanticType(const IdRef &ref, SemanticType type) {
	if (curMajorContext.isImport)
		return;

	for (size_t i = 0; i < ref.size(); ++i) {
		if (ref[i].idxAccessOpToken != SIZE_MAX) {
			tokenInfos[ref[i].idxAccessOpToken].semanticType = type;
		}
		if (ref[i].idxToken != SIZE_MAX) {
			tokenInfos[ref[i].idxToken].semanticType = type;
		}
	}
}

void Compiler::updateTokenInfo(size_t idxToken, std::function<void(TokenInfo &info)> updater) {
	if (curMajorContext.isImport)
		return;

	if (idxToken == SIZE_MAX)
		return;

	updater(tokenInfos[idxToken]);
}

#endif
