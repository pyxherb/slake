#include "../compiler.h"

using namespace slake::slkc;

std::deque<std::shared_ptr<FnOverloadingNode>> Compiler::argDependentLookup(
	Location loc,
	FnNode *fn,
	const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
	const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs) {
	std::deque<std::shared_ptr<FnOverloadingNode>> matchedRegistries;

	for (auto i : fn->overloadingRegistries) {
		auto overloading = i;
		size_t nParams = overloading->params.size(), nGenericParams = overloading->genericParams.size();

		if (overloading->isVaridic())
			--nParams;

		if (nParams > argTypes.size())
			continue;

		if (nParams < argTypes.size()) {
			if (!overloading->isVaridic())
				continue;
		}

		if (nGenericParams != genericArgs.size())
			continue;

		if (nGenericParams) {
			GenericNodeInstantiationContext instantiationContext{ &genericArgs, {} };
			overloading = instantiateGenericFnOverloading(overloading, instantiationContext);
		}

		bool exactlyMatched = true;

		for (size_t j = 0; j < nParams; ++j) {
			if (!isSameType(overloading->params[j]->type, argTypes[j])) {
				exactlyMatched = false;

				if (!isTypeNamesConvertible(argTypes[j], overloading->params[j]->type))
					goto fail;
			}
		}

		if (exactlyMatched)
			return { overloading };

		matchedRegistries.push_back(overloading);
	fail:;
	}

	return matchedRegistries;
}
