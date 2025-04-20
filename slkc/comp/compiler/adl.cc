#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::determineFnOverloading(
	CompileContext *compileContext,
	peff::SharedPtr<FnSlotNode> fnSlot,
	const peff::SharedPtr<TypeNameNode> *argTypes,
	size_t nArgTypes,
	bool isStatic,
	peff::DynArray<size_t> &matchedOverloadings) {
	for (size_t i = 0; i < fnSlot->overloadings.size(); ++i) {
		bool exactlyMatched = false;
		peff::SharedPtr<FnNode> currentOverloading = fnSlot->overloadings.at(i);

		if (isStatic == ((currentOverloading->accessModifier & slake::ACCESS_STATIC) == slake::ACCESS_STATIC)) {
			continue;
		}

		if (nArgTypes < currentOverloading->params.size()) {
			continue;
		} else if (nArgTypes > currentOverloading->params.size()) {
			if (!(currentOverloading->fnFlags & FN_VARG)) {
				continue;
			}
		}

		for (size_t j = 0; j < currentOverloading->params.size(); ++j) {
			peff::SharedPtr<VarNode> currentParam = currentOverloading->params.at(j);

			bool whether = false;
			SLKC_RETURN_IF_COMP_ERROR(isSameType(currentParam->type, argTypes[i], whether));

			if (!whether) {
				SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(argTypes[i], currentParam->type, whether));
				if (!whether) {
					continue;
				}
				exactlyMatched = false;
			}

			if (exactlyMatched) {
				matchedOverloadings.clear();

				if (!matchedOverloadings.pushBack(+j)) {
					return genOutOfMemoryCompError();
				}

				return {};
			} else {
				if (!matchedOverloadings.pushBack(+j)) {
					return genOutOfMemoryCompError();
				}
			}
		}
	}

	return {};
}
