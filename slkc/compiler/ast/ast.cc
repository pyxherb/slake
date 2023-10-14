#include "ast.h"

using namespace slake::slkc;

bool slake::slkc::isMemberNode(shared_ptr<AstNode> node) {
    switch(node->getNodeType()) {
        case AST_CLASS:
        case AST_INTERFACE:
        case AST_TRAIT:
        case AST_FN:
        case AST_MODULE:
        case AST_ALIAS:
        case AST_VAR:
            return true;
        case AST_STMT:
        case AST_EXPR:
        case AST_TYPENAME:
            return false;
        default:
            throw std::logic_error("UNrecognized node type");
    }
}