#include "compiler.h"

using namespace slkc;

SLKC_API TopLevelCompileContext::~TopLevelCompileContext() {
}

SLKC_API void TopLevelCompileContext::onRefZero() noexcept {
	peff::destroyAndRelease<TopLevelCompileContext>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLKC_API std::optional<slkc::CompilationError> slkc::typeNameCmp(TopLevelCompileContext *compileContext, peff::SharedPtr<TypeNameNode> lhs, peff::SharedPtr<TypeNameNode> rhs, int &out) noexcept {
	if (((uint8_t)lhs->typeNameKind) < ((uint8_t)rhs->typeNameKind)) {
		out = -1;
		return {};
	}
	if (((uint8_t)lhs->typeNameKind) > ((uint8_t)rhs->typeNameKind)) {
		out = 1;
		return {};
	}
	switch (lhs->typeNameKind) {
		case TypeNameKind::Custom: {
			peff::SharedPtr<CustomTypeNameNode>
				l = lhs.castTo<CustomTypeNameNode>(),
				r = rhs.castTo<CustomTypeNameNode>();

			peff::SharedPtr<MemberNode>
				lm,
				rm;

			{
				CustomTypeNameResolveContext resolveContext(compileContext->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(Compiler::resolveCustomTypeName(compileContext, resolveContext, l, lm));
			}
			{
				CustomTypeNameResolveContext resolveContext(compileContext->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(Compiler::resolveCustomTypeName(compileContext, resolveContext, r, rm));
			}

			if (!lm) {
				if (rm) {
					// [Bad type] > [Regular custom type]
					out = 1;
					return {};
				}
				// [Bad type] == [Bad type]
				out = 0;
				return {};
			}
			if (!rm) {
				out = -1;
				return {};
			}
			if (lm < rm) {
				out = -1;
			} else if (lm > rm) {
				out = 1;
			} else {
				out = 0;
			}
			return {};
		}
		case TypeNameKind::Array: {
			return typeNameCmp(compileContext,
				lhs.castTo<ArrayTypeNameNode>()->elementType,
				rhs.castTo<ArrayTypeNameNode>()->elementType,
				out);
		}
		case TypeNameKind::Ref: {
			return typeNameCmp(compileContext,
				lhs.castTo<RefTypeNameNode>()->referencedType,
				rhs.castTo<RefTypeNameNode>()->referencedType,
				out);
		}
		default:
			out = 0;
			return {};
	}

	std::terminate();
}

SLKC_API std::optional<slkc::CompilationError> slkc::typeNameListCmp(TopLevelCompileContext *compileContext, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs, int &out) noexcept {
	if (lhs.size() < rhs.size()) {
		out = -1;
		return {};
	}
	if (lhs.size() > rhs.size()) {
		out = 1;
		return {};
	}
	for (size_t i = 0; i < lhs.size(); ++i) {
		SLKC_RETURN_IF_COMP_ERROR(typeNameCmp(compileContext, lhs.at(i), rhs.at(i), out));

		if (out != 0) {
			return {};
		}
	}

	out = 0;
	return {};
}
