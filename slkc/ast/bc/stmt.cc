#include "stmt.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCStmtNode::BCStmtNode(BCStmtKind stmtKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::BCStmt, selfAllocator, document), stmtKind(stmtKind) {
}

SLKC_API BCStmtNode::~BCStmtNode() {
}

SLKC_API InstructionBCStmtNode::InstructionBCStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : BCStmtNode(BCStmtKind::Instruction, selfAllocator, document), mnemonic(selfAllocator), operands(selfAllocator) {
}

SLKC_API InstructionBCStmtNode::~InstructionBCStmtNode() {
}

SLKC_API LabelBCStmtNode::LabelBCStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : BCStmtNode(BCStmtKind::Label, selfAllocator, document), name(selfAllocator) {
}

SLKC_API LabelBCStmtNode::~LabelBCStmtNode() {
}

SLKC_API BadBCStmtNode::BadBCStmtNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<BCStmtNode> &body) : BCStmtNode(BCStmtKind::Bad, selfAllocator, document), body(body) {
}

SLKC_API BadBCStmtNode::~BadBCStmtNode() {
}
