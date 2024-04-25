#include "../compiler.h"

using namespace slake::slkc;

void Compiler::updateCorrespondingTokenInfo(
	shared_ptr<TypeNameNode> targetTypeName,
	shared_ptr<TypeNameNode> semanticType,
	CompletionContext completionContext) {
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
			tokenInfos[t->idxToken].semanticType = SemanticType::Type;
			tokenInfos[t->idxToken].semanticInfo.type = semanticType;
			tokenInfos[t->idxToken].completionContext = completionContext;
			tokenInfos[t->idxToken].tokenContext = TokenContext(curFn, curMajorContext);
			break;
		}
		case Type::Array: {
			auto t = static_pointer_cast<ArrayTypeNameNode>(targetTypeName);
			updateCorrespondingTokenInfo(t->elementType, semanticType, completionContext);
			break;
		}
		case Type::Custom: {
			auto t = static_pointer_cast<CustomTypeNameNode>(targetTypeName);

			updateCorrespondingTokenInfo(t->ref, SemanticType::TypeRef, completionContext);
			break;
		}
		default:
			assert(false);
	}
}

void Compiler::updateCorrespondingTokenInfo(const Ref &ref, SemanticType semanticType, CompletionContext completionContext) {
	deque<pair<Ref, shared_ptr<AstNode>>> partsOut;
	if (!resolveRef(ref, partsOut))
		return;

	for (size_t i = 0; i < ref.size(); ++i) {
		if (ref[i].idxToken != SIZE_MAX) {
			tokenInfos[ref[i].idxToken].semanticType = semanticType;
			tokenInfos[ref[i].idxToken].completionContext = completionContext;
		}
	}
}
