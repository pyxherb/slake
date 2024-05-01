#include "../compiler.h"

using namespace slake::slkc;

#if SLKC_WITH_LANGUAGE_SERVER

void Compiler::updateCompletionContext(size_t idxToken, CompletionContext completionContext) {
	tokenInfos[idxToken].completionContext = completionContext;
}

void Compiler::updateCompletionContext(shared_ptr<TypeNameNode> targetTypeName, CompletionContext completionContext) {
	switch (targetTypeName->getTypeId()) {
		case Type::I8:
		case Type::I16:
		case Type::I32:
		case Type::I64:
		case Type::U8:
		case Type::U16:
		case Type::U32:
		case Type::U64:
		case Type::F32:
		case Type::F64:
		case Type::String:
		case Type::Bool:
		case Type::Void:
		case Type::Any:
		case Type::Auto: {
			auto t = static_pointer_cast<BasicSimpleTypeNameNode>(targetTypeName);
			assert(t->idxToken != SIZE_MAX);
			tokenInfos[t->idxToken].completionContext = completionContext;
			break;
		}
		case Type::Array: {
			auto t = static_pointer_cast<ArrayTypeNameNode>(targetTypeName);
			updateCompletionContext(t->elementType, completionContext);
			break;
		}
		case Type::Custom: {
			auto t = static_pointer_cast<CustomTypeNameNode>(targetTypeName);

			updateCompletionContext(t->ref, completionContext);
			break;
		}
		case Type::Bad: {
			auto t = static_pointer_cast<BadTypeNameNode>(targetTypeName);

			for (size_t i = t->idxStartToken; i <= t->idxEndToken; ++i)
				tokenInfos[i].completionContext = completionContext;

			break;
		}
		default:
			assert(false);
	}
}

void Compiler::updateCompletionContext(const Ref &ref, CompletionContext completionContext) {
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
	tokenInfos[idxToken].semanticType = type;
}

void Compiler::updateSemanticType(shared_ptr<TypeNameNode> targetTypeName, SemanticType type) {
	switch (targetTypeName->getTypeId()) {
		case Type::I8:
		case Type::I16:
		case Type::I32:
		case Type::I64:
		case Type::U8:
		case Type::U16:
		case Type::U32:
		case Type::U64:
		case Type::F32:
		case Type::F64:
		case Type::String:
		case Type::Bool:
		case Type::Void:
		case Type::Any:
		case Type::Auto: {
			auto t = static_pointer_cast<BasicSimpleTypeNameNode>(targetTypeName);
			assert(t->idxToken != SIZE_MAX);
			tokenInfos[t->idxToken].semanticType = type;
			break;
		}
		case Type::Array: {
			auto t = static_pointer_cast<ArrayTypeNameNode>(targetTypeName);
			updateSemanticType(t->elementType, type);
			break;
		}
		case Type::Custom: {
			auto t = static_pointer_cast<CustomTypeNameNode>(targetTypeName);

			updateSemanticType(t->ref, type);
			break;
		}
		case Type::Bad: {
			auto t = static_pointer_cast<BadTypeNameNode>(targetTypeName);

			for (size_t i = t->idxStartToken; i <= t->idxEndToken; ++i)
				tokenInfos[i].semanticType = type;

			break;
		}
		default:
			assert(false);
	}
}

void Compiler::updateSemanticType(const Ref &ref, SemanticType type) {
	for (size_t i = 0; i < ref.size(); ++i) {
		if (ref[i].idxAccessOpToken != SIZE_MAX) {
			tokenInfos[ref[i].idxAccessOpToken].semanticType = type;
		}
		if (ref[i].idxToken != SIZE_MAX) {
			tokenInfos[ref[i].idxToken].semanticType = type;
		}
	}
}

#endif
