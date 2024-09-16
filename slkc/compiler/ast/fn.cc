#include "../compiler.h"

using namespace slake::slkc;

FnOverloadingNode::FnOverloadingNode(
	Compiler *compiler,
	std::shared_ptr<Scope> scope) : MemberNode(compiler, 0) {
	setScope(scope);  // For custom type names.
}

void FnOverloadingNode::updateParamIndices() {
	for (size_t i = 0; i < params.size(); ++i) {
		if (paramIndices.count(params[i]->name)) {
			throw FatalCompilationError(
				Message(
					compiler->tokenRangeToSourceLocation(params[i]->tokenRange),
					MessageType::Error,
					"Redefinition of parameter `" + params[i]->name + "'"));
		}

		paramIndices[params[i]->name] = i;
	}
}

IdRefEntry FnOverloadingNode::getName() const {
	IdRefEntry idRefEntry(tokenRange, SIZE_MAX, owner->name);

	idRefEntry.hasParamTypes = true;

	idRefEntry.paramTypes.resize(params.size());
	for (size_t i = 0; i < params.size(); ++i) {
		idRefEntry.paramTypes[i] = params[i]->type;
	}

	if (genericArgs.size()) {
		idRefEntry.genericArgs = genericArgs;
	} else
		idRefEntry.genericArgs = getPlaceholderGenericArgs();

	idRefEntry.hasVarArg = isVaridic();

	return idRefEntry;
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
