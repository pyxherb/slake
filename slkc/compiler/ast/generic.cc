#include <slkc/compiler/compiler.h>

using namespace slake::slkc;

shared_ptr<MemberNode> Compiler::instantiateGenericNode(shared_ptr<MemberNode> node, deque<shared_ptr<TypeNameNode>> genericArgs) {
	// TODO: Add support for generic caches.
	shared_ptr<MemberNode> newInstance = static_pointer_cast<MemberNode>(node->duplicate());
	
	newInstance->genericArgs = genericArgs;
	newInstance->uninstantiatedValue = node;
	
	return newInstance;
}
