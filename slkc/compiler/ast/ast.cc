#include "ast.h"

using namespace slake::slkc;

bool slake::slkc::isMemberNode(std::shared_ptr<AstNode> node) {
    switch(node->getNodeType()) {
        case NodeType::Class:
        case NodeType::Interface:
        case NodeType::Fn:
        case NodeType::Module:
        case NodeType::Alias:
        case NodeType::Var:
            return true;
        case NodeType::Stmt:
        case NodeType::Expr:
        case NodeType::TypeName:
            return false;
        default:
            throw std::logic_error("UNrecognized node type");
    }
}
