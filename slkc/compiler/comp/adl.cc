#include "../compiler.h"

using namespace slake::slkc;

FnOverloadingRegistry *Compiler::argDependentLookup(Location loc, FnNode *fn, const deque<shared_ptr<TypeNameNode>> &argTypes) {
	std::deque<FnOverloadingRegistry *> matchedRegistries;

	for (auto &i : fn->overloadingRegistries) {
		size_t nParams = i.params.size();

		if (i.isVaridic())
			--nParams;

		if (nParams > argTypes.size())
			continue;

		if (nParams < argTypes.size()) {
			if (!i.isVaridic())
				continue;
		}

		bool exactMatched = true;

		for (size_t j = 0; j < nParams; ++j) {
			if (!isSameType(i.params[j].type, argTypes[j])) {
				exactMatched = false;

				if (!areTypesConvertible(argTypes[j], i.params[j].type))
					goto fail;
			}
		}

		matchedRegistries.push_back(&i);

		if (exactMatched)
			return &i;
	fail:;
	}

	if (matchedRegistries.size() > 1) {
		for (auto i : matchedRegistries) {
			messages.push_back(
				Message(
					i->loc,
					MessageType::Note,
					"Matched here"));
		}
		throw FatalCompilationError(
			Message(
				loc,
				MessageType::Error,
				"Ambiguous function call"));
	} else if (matchedRegistries.empty()) {
		throw FatalCompilationError(
			Message(
				loc,
				MessageType::Error,
				"No matched function was found"));
	}

	return matchedRegistries.front();
}
