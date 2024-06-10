#include "../compiler.h"

using namespace slake::slkc;

FnOverloadingNode::FnOverloadingNode(
	Compiler *compiler,
	std::shared_ptr<Scope> scope) : MemberNode(compiler, 0), returnType(returnType), params(params), idxNameToken(idxNameToken) {
	setScope(scope);  // For custom type names.
}

void FnOverloadingNode::updateParamIndices() {
	for (size_t i = 0; i < params.size(); ++i) {
		if (paramIndices.count(params[i]->name)) {
			throw FatalCompilationError(
				Message(
					params[i]->sourceLocation,
					MessageType::Error,
					"Redefinition of parameter `" + params[i]->name + "'"));
		}

		paramIndices[params[i]->name] = i;
	}
}

IdRefEntry FnOverloadingNode::getName() const {
	if (genericArgs.size())
		return IdRefEntry(sourceLocation, SIZE_MAX, owner->name, genericArgs);
	return IdRefEntry(sourceLocation, SIZE_MAX, owner->name, getPlaceholderGenericArgs());
}

std::shared_ptr<AstNode> ParamNode::doDuplicate() {
	return std::make_shared<ParamNode>(*this);
}

std::shared_ptr<AstNode> FnOverloadingNode::doDuplicate() {
	return std::make_shared<FnOverloadingNode>(*this);
}

std::shared_ptr<AstNode> FnNode::doDuplicate() {
	return std::make_shared<FnNode>(*this);
}

std::shared_ptr<AstNode> CompiledFnNode::doDuplicate() {
	return std::make_shared<CompiledFnNode>(*this);
}
