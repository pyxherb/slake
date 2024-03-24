#include <slkc/compiler/compiler.h>

using namespace slake::slkc;

shared_ptr<MemberNode> Compiler::instantiateGenericNode(shared_ptr<MemberNode> node, deque<shared_ptr<TypeNameNode>> genericArgs) {
	if (auto it = _genericCacheDir.find(node.get()); it != _genericCacheDir.end()) {
		if (auto subIt = it->second.find(genericArgs); subIt != it->second.end())
			return static_pointer_cast<MemberNode>(subIt->second->getSharedPtr());
	}

	shared_ptr<MemberNode> newInstance = static_pointer_cast<MemberNode>(node->duplicate());

	newInstance->genericArgs = genericArgs;
	newInstance->originalValue = node.get();

	_genericCacheDir[node.get()][genericArgs] = newInstance.get();

	return newInstance;
}
