#ifndef _SLKC_AST_BC_STMT_H_
#define _SLKC_AST_BC_STMT_H_

#include "../expr.h"
#include "../generic.h"

namespace slkc {
	namespace bc {
		enum class BCStmtKind : uint8_t {
			Instruction = 0,  // Instruction
			Label,			  // Label

			Bad,  // Bad statement - unrecognized statement type
		};

		class BCStmtNode : public AstNode {
		public:
			BCStmtKind stmt_kind;

			SLKC_API BCStmtNode(BCStmtKind stmt_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~BCStmtNode();
		};

		class InstructionBCStmtNode : public BCStmtNode {
		public:
			uint32_t line = UINT32_MAX, column = UINT32_MAX;
			uint32_t reg_out = UINT32_MAX;
			peff::String mnemonic;
			peff::DynArray<AstNodePtr<ExprNode>> operands;

			SLKC_API InstructionBCStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~InstructionBCStmtNode();
		};

		class LabelBCStmtNode : public BCStmtNode {
		public:
			peff::String name;

			SLKC_API LabelBCStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~LabelBCStmtNode();
		};

		class BadBCStmtNode : public BCStmtNode {
		public:
			AstNodePtr<BCStmtNode> body;

			SLKC_API BadBCStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<BCStmtNode> &body);
			SLKC_API virtual ~BadBCStmtNode();
		};
	}
}

#endif
