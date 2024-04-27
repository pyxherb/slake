#include "../compiler.h"

using namespace slake::slkc;

FnOverloadingNode::FnOverloadingNode(
	Location loc,
	Compiler *compiler,
	shared_ptr<TypeNameNode> returnType,
	GenericParamNodeList genericParams,
	deque<shared_ptr<ParamNode>> params,
	shared_ptr<Scope> scope,
	size_t idxNameToken) : MemberNode(compiler, access), returnType(returnType), params(params), idxNameToken(idxNameToken) {
	for (size_t i = 0; i < params.size(); ++i) {
		if (paramIndices.count(params[i]->name)) {
			throw FatalCompilationError(
				Message(
					params[i]->loc,
					MessageType::Error,
					"Redefinition of parameter `" + params[i]->name + "'"));
		}

		paramIndices[params[i]->name] = i;
	}

	setGenericParams(genericParams);
	setScope(scope);  // For custom type names.
}

shared_ptr<AstNode> ParamNode::doDuplicate() {
	return make_shared<ParamNode>(*this);
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
