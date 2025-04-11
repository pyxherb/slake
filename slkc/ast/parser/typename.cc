#include "../parser.h"
#include <climits>

using namespace slkc;

SLKC_API std::optional<SyntaxError> Parser::parseTypeName(peff::SharedPtr<TypeNameNode> &typeNameOut) {
	std::optional<SyntaxError> syntaxError;
	Token *t = peekToken();

	switch (t->tokenId) {
		case TokenId::VoidTypeName:
			if (!(typeNameOut = peff::makeShared<VoidTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I8TypeName:
			if (!(typeNameOut = peff::makeShared<I8TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I16TypeName:
			if (!(typeNameOut = peff::makeShared<I16TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I32TypeName:
			if (!(typeNameOut = peff::makeShared<I32TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::I64TypeName:
			if (!(typeNameOut = peff::makeShared<I64TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U8TypeName:
			if (!(typeNameOut = peff::makeShared<U8TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U16TypeName:
			if (!(typeNameOut = peff::makeShared<U16TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U32TypeName:
			if (!(typeNameOut = peff::makeShared<U32TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::U64TypeName:
			if (!(typeNameOut = peff::makeShared<U64TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::ISizeTypeName:
			if (!(typeNameOut = peff::makeShared<ISizeTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			break;
		case TokenId::USizeTypeName:
			if (!(typeNameOut = peff::makeShared<USizeTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::F32TypeName:
			if (!(typeNameOut = peff::makeShared<F32TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::F64TypeName:
			if (!(typeNameOut = peff::makeShared<F64TypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::BoolTypeName:
			if (!(typeNameOut = peff::makeShared<BoolTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(), document)
						.castTo<TypeNameNode>()))
				return genOutOfMemoryError();
			typeNameOut->tokenRange = TokenRange{ t->index };
			nextToken();
			break;
		case TokenId::Id: {
			IdRefPtr id;
			if ((syntaxError = parseIdRef(id)))
				return syntaxError;

			peff::SharedPtr<CustomTypeNameNode> tn;

			if (!(tn = peff::makeShared<CustomTypeNameNode>(
					  resourceAllocator.get(),
					  resourceAllocator.get(),
					  document)))
				return genOutOfMemoryError();

			tn->contextNode = curParent;

			tn->tokenRange = id->tokenRange;
			tn->idRefPtr = std::move(id);

			typeNameOut = tn.castTo<TypeNameNode>();

			break;
		}
		default:
			return SyntaxError(TokenRange{ t->index }, SyntaxErrorKind::UnexpectedToken);
	}

	while (true) {
		switch ((t = peekToken())->tokenId) {
			case TokenId::LBracket: {
				nextToken();

				Token *rBracketToken;
				if ((syntaxError = expectToken((rBracketToken = peekToken()), TokenId::RBracket)))
					return SyntaxError(TokenRange{ rBracketToken->index }, ExpectingSingleTokenErrorExData{ TokenId::RBracket });

				nextToken();

				if (!(typeNameOut = peff::makeShared<ArrayTypeNameNode>(
						  resourceAllocator.get(),
						  resourceAllocator.get(),
						  document,
						  typeNameOut)
							.castTo<TypeNameNode>()))
					return genOutOfMemoryError();
				break;
			}
			default:
				goto end;
		}
	}

end:
	if ((t = peekToken())->tokenId == TokenId::AndOp) {
		nextToken();
		if (!(typeNameOut = peff::makeShared<RefTypeNameNode>(
				  resourceAllocator.get(),
				  resourceAllocator.get(),
				  document,
				  typeNameOut)
					.castTo<TypeNameNode>()))
			return genOutOfMemoryError();
	}

	return {};
}
