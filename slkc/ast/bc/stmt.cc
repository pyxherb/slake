#include "stmt.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCStmtNode::BCStmtNode(BCStmtKind stmt_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::BCStmt, self_allocator, document), stmt_kind(stmt_kind) {
}

SLKC_API BCStmtNode::~BCStmtNode() {
}

SLKC_API InstructionBCStmtNode::InstructionBCStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : BCStmtNode(BCStmtKind::Instruction, self_allocator, document), mnemonic(self_allocator), operands(self_allocator) {
}

SLKC_API InstructionBCStmtNode::~InstructionBCStmtNode() {
}

SLKC_API LabelBCStmtNode::LabelBCStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : BCStmtNode(BCStmtKind::Label, self_allocator, document), name(self_allocator) {
}

SLKC_API LabelBCStmtNode::~LabelBCStmtNode() {
}

SLKC_API BadBCStmtNode::BadBCStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<BCStmtNode> &body) : BCStmtNode(BCStmtKind::Bad, self_allocator, document), body(body) {
}

SLKC_API BadBCStmtNode::~BadBCStmtNode() {
}
