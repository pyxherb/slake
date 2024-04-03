#include "../compiler.h"

using namespace slake::slkc;

FnOverloadingNode::FnOverloadingNode(
	Location loc,
	Compiler *compiler,
	shared_ptr<TypeNameNode> returnType,
	GenericParamNodeList genericParams,
	deque<Param> params,
	shared_ptr<Scope> scope) : MemberNode(compiler, access), returnType(returnType), params(params) {
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

	setGenericParams(genericParams);
	setScope(scope);  // For custom type names.
}

shared_ptr<AstNode> FnOverloadingNode::doDuplicate() {
	return make_shared<FnOverloadingNode>(*this);
}

shared_ptr<AstNode> FnNode::doDuplicate() {
	return make_shared<FnNode>(*this);
}

shared_ptr<AstNode> CompiledFnNode::doDuplicate() {
	return make_shared<CompiledFnNode>(*this);
}
