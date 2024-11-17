#include "../compiler.h"

using namespace slake::slkc;

void slake::slkc::Compiler::_argDependentLookup(
	CompileContext *compileContext,
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

			if (!isSameType(compileContext, curType, genericArgs[j]))
				goto fail;
		}

		if (nGenericParams) {
			GenericNodeInstantiationContext instantiationContext{ &genericArgs, {} };
			i = instantiateGenericFnOverloading(i, instantiationContext);
		}

		{
			bool exactlyMatched = true;

			for (size_t j = 0; j < nParams; ++j) {
				if (!isSameType(compileContext, i->params[j]->type, argTypes[j])) {
					exactlyMatched = false;

					if (!isTypeNamesConvertible(compileContext, argTypes[j], i->params[j]->type))
						goto fail;
				}
			}

			if (exactlyMatched) {
				overloadingsOut = { i };
				return;
			}
		}

		overloadingsOut.push_back(i);
	fail:;
	}

	if ((!isStatic) && fn->parentFn)
		_argDependentLookup(compileContext, fn->parentFn, argTypes, genericArgs, overloadingsOut, isStatic);
}

std::deque<std::shared_ptr<FnOverloadingNode>> Compiler::argDependentLookup(
	CompileContext *compileContext,
	FnNode *fn,
	const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
	const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
	bool isStatic) {
	std::deque<std::shared_ptr<FnOverloadingNode>> matchedRegistries;

	_argDependentLookup(compileContext, fn, argTypes, genericArgs, matchedRegistries, isStatic);

	return matchedRegistries;
}
