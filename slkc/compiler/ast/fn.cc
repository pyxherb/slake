#include "../compiler.h"

using namespace slake::slkc;

slake::slkc::FnOverloadingRegistry::FnOverloadingRegistry(
	Location loc,
	shared_ptr<TypeNameNode> returnType,
	GenericParamNodeList genericParams,
	deque<Param> params)
	: loc(loc), returnType(returnType), genericParams(genericParams), params(params) {
	for (size_t i = 0; i < params.size(); ++i) {
		if (paramIndices.count(params[i].name)) {
			throw FatalCompilationError(
				Message(
					params[i].loc,
					MessageType::Error,
					"Redefinition of parameter `" + params[i].name + "'"));
		}

		paramIndices[params[i].name] = i;
	}
}
