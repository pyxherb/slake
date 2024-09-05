#include "../compiler.h"

using namespace slake::slkc;

void slake::slkc::Compiler::_argDependentLookup(
	FnNode *fn,
	const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
	const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
	std::deque<std::shared_ptr<FnOverloadingNode>> &overloadingsOut,
	bool isStatic) {
	for (auto i : fn->overloadingRegistries) {
		if (isStatic) {
			if (!(i->access & ACCESS_STATIC))
				continue;
		} else {
			if (i->access & ACCESS_STATIC)
				continue;
		}

		size_t nParams = i->params.size(), nGenericParams = i->genericParams.size();

		if (i->isVaridic())
			--nParams;

		if (nParams > argTypes.size())
			continue;

		if (nParams < argTypes.size()) {
			if (!i->isVaridic())
				continue;
		}

		if (nGenericParams != genericArgs.size())
			continue;

		for (size_t j = 0; j < i->specializationArgs.size(); ++j) {
			std::shared_ptr<TypeNameNode> curType = i->specializationArgs[j];

			if (!isSameType(curType, genericArgs[j]))
				goto fail;
		}

		if (nGenericParams) {
			GenericNodeInstantiationContext instantiationContext{ &genericArgs, {} };
			i = instantiateGenericFnOverloading(i, instantiationContext);
		}

		bool exactlyMatched = true;

		for (size_t j = 0; j < nParams; ++j) {
			if (!isSameType(i->params[j]->type, argTypes[j])) {
				exactlyMatched = false;

				if (!isTypeNamesConvertible(argTypes[j], i->params[j]->type))
					goto fail;
			}
		}

		if (exactlyMatched) {
			overloadingsOut = { i };
			return;
		}

		overloadingsOut.push_back(i);
	fail:;
	}

	if ((!isStatic) && fn->parentFn)
		_argDependentLookup(fn->parentFn, argTypes, genericArgs, overloadingsOut, isStatic);
}

std::deque<std::shared_ptr<FnOverloadingNode>> Compiler::argDependentLookup(
	FnNode *fn,
	const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
	const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
	bool isStatic) {
	std::deque<std::shared_ptr<FnOverloadingNode>> matchedRegistries;

	_argDependentLookup(fn, argTypes, genericArgs, matchedRegistries, isStatic);

	return matchedRegistries;
}
