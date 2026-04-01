#include "expr.h"

using namespace slkc;

SLKC_API ExprNode::ExprNode(ExprKind expr_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::Expr, self_allocator, document), expr_kind(expr_kind) {
}

SLKC_API ExprNode::ExprNode(const ExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : AstNode(rhs, allocator, context), expr_kind(rhs.expr_kind) {
}

SLKC_API ExprNode::~ExprNode() {
}

SLKC_API AstNodePtr<AstNode> UnaryExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;

	AstNodePtr<UnaryExprNode> duplicated_node(make_ast_node<UnaryExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API UnaryExprNode::UnaryExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Unary, self_allocator, document) {
}

SLKC_API UnaryExprNode::UnaryExprNode(const UnaryExprNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, self_allocator, context), unary_op(rhs.unary_op) {
	if (!context.push_task([this, &rhs, self_allocator, &context]() -> bool {
			if (!(operand = rhs.operand->duplicate<ExprNode>(self_allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API UnaryExprNode::~UnaryExprNode() {
}

SLKC_API const char *slkc::get_binary_operator_overloading_name(BinaryOp op) {
	switch (op) {
		case BinaryOp::Add:
			return "+";
		case BinaryOp::Sub:
			return "-";
		case BinaryOp::Mul:
			return "*";
		case BinaryOp::Div:
			return "/";
		case BinaryOp::Mod:
			return "%";
		case BinaryOp::And:
			return "&";
		case BinaryOp::Or:
			return "|";
		case BinaryOp::Xor:
			return "^";
		case BinaryOp::LAnd:
			return "&&";
		case BinaryOp::LOr:
			return "||";
		case BinaryOp::Shl:
			return "<<";
		case BinaryOp::Shr:
			return ">>";
		case BinaryOp::AddAssign:
			return "+=";
		case BinaryOp::SubAssign:
			return "-=";
		case BinaryOp::MulAssign:
			return "*=";
		case BinaryOp::DivAssign:
			return "/=";
		case BinaryOp::ModAssign:
			return "%=";
		case BinaryOp::AndAssign:
			return "&=";
		case BinaryOp::OrAssign:
			return "|=";
		case BinaryOp::XorAssign:
			return "^=";
		case BinaryOp::ShlAssign:
			return "<<=";
		case BinaryOp::ShrAssign:
			return ">>=";
		case BinaryOp::Eq:
			return "==";
		case BinaryOp::Neq:
			return "!=";
		case BinaryOp::Lt:
			return "<";
		case BinaryOp::Gt:
			return ">";
		case BinaryOp::LtEq:
			return "<=";
		case BinaryOp::GtEq:
			return ">=";
		case BinaryOp::Cmp:
			return "<=>";
		case BinaryOp::Subscript:
			return "[]";
		default:
			break;
	}

	return nullptr;
}

SLKC_API AstNodePtr<AstNode> BinaryExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<BinaryExprNode> duplicated_node(make_ast_node<BinaryExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API BinaryExprNode::BinaryExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Binary, self_allocator, document) {
}

SLKC_API BinaryExprNode::BinaryExprNode(const BinaryExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context), binary_op(rhs.binary_op) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(this->lhs = rhs.lhs->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(this->rhs = rhs.rhs->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API BinaryExprNode::~BinaryExprNode() {
}

SLKC_API AstNodePtr<AstNode> TernaryExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<TernaryExprNode> duplicated_node(make_ast_node<TernaryExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API TernaryExprNode::TernaryExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Ternary, self_allocator, document) {
}

SLKC_API TernaryExprNode::TernaryExprNode(const TernaryExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(this->cond = rhs.cond->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(this->lhs = rhs.lhs->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(this->rhs = rhs.rhs->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API TernaryExprNode::~TernaryExprNode() {
}

SLKC_API AstNodePtr<AstNode> LooseIdExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<LooseIdExprNode> duplicated_node(make_ast_node<LooseIdExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API LooseIdExprNode::LooseIdExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	peff::String &&id)
	: ExprNode(ExprKind::IdRef, self_allocator, document),
	  id(std::move(id)) {
}
SLKC_API LooseIdExprNode::LooseIdExprNode(const LooseIdExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context), id(allocator) {
	if (!(id.build(rhs.id))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}
SLKC_API LooseIdExprNode::~LooseIdExprNode() {
}

SLKC_API AstNodePtr<AstNode> IdRefExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<IdRefExprNode> duplicated_node(make_ast_node<IdRefExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API IdRefExprNode::IdRefExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	IdRefPtr &&id_ref_ptr)
	: ExprNode(ExprKind::IdRef, self_allocator, document),
	  id_ref_ptr(std::move(id_ref_ptr)) {
}
SLKC_API IdRefExprNode::IdRefExprNode(const IdRefExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context) {
	if (!(id_ref_ptr = duplicate_id_ref(allocator, rhs.id_ref_ptr.get()))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}
SLKC_API IdRefExprNode::~IdRefExprNode() {
}

SLKC_API AstNodePtr<AstNode> HeadedIdRefExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<HeadedIdRefExprNode> duplicated_node(make_ast_node<HeadedIdRefExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API HeadedIdRefExprNode::HeadedIdRefExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	const AstNodePtr<ExprNode> &head,
	IdRefPtr &&id_ref_ptr)
	: ExprNode(ExprKind::HeadedIdRef, self_allocator, document),
	  id_ref_ptr(std::move(id_ref_ptr)) {
}
SLKC_API HeadedIdRefExprNode::HeadedIdRefExprNode(const HeadedIdRefExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(this->head = rhs.head->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}
	if (!(id_ref_ptr = duplicate_id_ref(allocator, rhs.id_ref_ptr.get()))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}
SLKC_API HeadedIdRefExprNode::~HeadedIdRefExprNode() {
}

SLKC_API AstNodePtr<AstNode> I8LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I8LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<I8LiteralExprNode, AstNodeControlBlock<I8LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I8LiteralExprNode::I8LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	int8_t data)
	: ExprNode(ExprKind::I8, self_allocator, document),
	  data(data) {
}
SLKC_API I8LiteralExprNode::I8LiteralExprNode(const I8LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API I8LiteralExprNode::~I8LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> I16LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I16LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<I16LiteralExprNode, AstNodeControlBlock<I16LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I16LiteralExprNode::I16LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	int16_t data)
	: ExprNode(ExprKind::I16, self_allocator, document),
	  data(data) {
}
SLKC_API I16LiteralExprNode::I16LiteralExprNode(const I16LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API I16LiteralExprNode::~I16LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> I32LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I32LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<I32LiteralExprNode, AstNodeControlBlock<I32LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I32LiteralExprNode::I32LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	int32_t data)
	: ExprNode(ExprKind::I32, self_allocator, document),
	  data(data) {
}
SLKC_API I32LiteralExprNode::I32LiteralExprNode(const I32LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API I32LiteralExprNode::~I32LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> I64LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I64LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<I64LiteralExprNode, AstNodeControlBlock<I64LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I64LiteralExprNode::I64LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	int64_t data)
	: ExprNode(ExprKind::I64, self_allocator, document),
	  data(data) {
}
SLKC_API I64LiteralExprNode::I64LiteralExprNode(const I64LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API I64LiteralExprNode::~I64LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U8LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U8LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<U8LiteralExprNode, AstNodeControlBlock<U8LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U8LiteralExprNode::U8LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	uint8_t data)
	: ExprNode(ExprKind::U8, self_allocator, document),
	  data(data) {
}
SLKC_API U8LiteralExprNode::U8LiteralExprNode(const U8LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API U8LiteralExprNode::~U8LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U16LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U16LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<U16LiteralExprNode, AstNodeControlBlock<U16LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U16LiteralExprNode::U16LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	uint16_t data)
	: ExprNode(ExprKind::U16, self_allocator, document),
	  data(data) {
}
SLKC_API U16LiteralExprNode::U16LiteralExprNode(const U16LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API U16LiteralExprNode::~U16LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U32LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U32LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<U32LiteralExprNode, AstNodeControlBlock<U32LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API U32LiteralExprNode::U32LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	uint32_t data)
	: ExprNode(ExprKind::U32, self_allocator, document),
	  data(data) {
}
SLKC_API U32LiteralExprNode::U32LiteralExprNode(const U32LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API U32LiteralExprNode::~U32LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U64LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U64LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<U64LiteralExprNode, AstNodeControlBlock<U64LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U64LiteralExprNode::U64LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	uint64_t data)
	: ExprNode(ExprKind::U64, self_allocator, document),
	  data(data) {
}
SLKC_API U64LiteralExprNode::U64LiteralExprNode(const U64LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API U64LiteralExprNode::~U64LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> F32LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<F32LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API F32LiteralExprNode::F32LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	float data)
	: ExprNode(ExprKind::F32, self_allocator, document),
	  data(data) {
}
SLKC_API F32LiteralExprNode::F32LiteralExprNode(const F32LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API F32LiteralExprNode::~F32LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> F64LiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<F64LiteralExprNode> duplicated_node(peff::make_shared_with_control_block<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API F64LiteralExprNode::F64LiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	double data)
	: ExprNode(ExprKind::F64, self_allocator, document),
	  data(data) {
}
SLKC_API F64LiteralExprNode::F64LiteralExprNode(const F64LiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API F64LiteralExprNode::~F64LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> BoolLiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<BoolLiteralExprNode> duplicated_node(make_ast_node<BoolLiteralExprNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API BoolLiteralExprNode::BoolLiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	bool data)
	: ExprNode(ExprKind::Bool, self_allocator, document),
	  data(data) {
}
SLKC_API BoolLiteralExprNode::BoolLiteralExprNode(const BoolLiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context), data(rhs.data) {
}
SLKC_API BoolLiteralExprNode::~BoolLiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> StringLiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<StringLiteralExprNode> duplicated_node(make_ast_node<StringLiteralExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API StringLiteralExprNode::StringLiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	peff::String &&data)
	: ExprNode(ExprKind::String, self_allocator, document),
	  data(std::move(data)) {
}
SLKC_API StringLiteralExprNode::StringLiteralExprNode(const StringLiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context), data(allocator) {
	if (!data.build(rhs.data)) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}
SLKC_API StringLiteralExprNode::~StringLiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> NullLiteralExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<NullLiteralExprNode> duplicated_node(make_ast_node<NullLiteralExprNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API NullLiteralExprNode::NullLiteralExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Null, self_allocator, document) {
}
SLKC_API NullLiteralExprNode::NullLiteralExprNode(const NullLiteralExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : ExprNode(rhs, allocator, context) {
}
SLKC_API NullLiteralExprNode::~NullLiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> InitializerListExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<InitializerListExprNode> duplicated_node(make_ast_node<InitializerListExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API InitializerListExprNode::InitializerListExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::InitializerList, self_allocator, document),
	  elements(self_allocator) {
}
SLKC_API InitializerListExprNode::InitializerListExprNode(const InitializerListExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ExprNode(rhs, allocator, context), elements(allocator) {
	if (!elements.resize(rhs.elements.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < elements.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(elements.at(i) = rhs.elements.at(i)->duplicate<ExprNode>(allocator))) {
					return false;
				}
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}
SLKC_API InitializerListExprNode::~InitializerListExprNode() {
}

SLKC_API AstNodePtr<AstNode> CallExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<CallExprNode> duplicated_node(make_ast_node<CallExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API CallExprNode::CallExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	const AstNodePtr<ExprNode> &target,
	peff::DynArray<AstNodePtr<ExprNode>> &&args)
	: ExprNode(ExprKind::Call, self_allocator, document),
	  target(target),
	  args(std::move(args)),
	  idx_comma_tokens(self_allocator) {
}
SLKC_API CallExprNode::CallExprNode(const CallExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context), args(allocator), idx_comma_tokens(allocator) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.target && !(target = rhs.target->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.with_object && !(with_object = rhs.with_object->duplicate<ExprNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!args.resize(rhs.args.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < args.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(args.at(i) = rhs.args.at(i)->duplicate<ExprNode>(allocator))) {
					return false;
				}
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}
}
SLKC_API CallExprNode::~CallExprNode() {
}

SLKC_API AstNodePtr<AstNode> NewExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<NewExprNode> duplicated_node(make_ast_node<NewExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API NewExprNode::NewExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::New, self_allocator, document),
	  args(self_allocator),
	  idx_comma_tokens(self_allocator) {
}
SLKC_API NewExprNode::NewExprNode(const NewExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context), args(allocator), idx_comma_tokens(allocator) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(target_type = rhs.target_type->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!args.resize(rhs.args.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < args.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(args.at(i) = rhs.args.at(i)->duplicate<ExprNode>(allocator))) {
					return false;
				}
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}
}
SLKC_API NewExprNode::~NewExprNode() {
}

SLKC_API AstNodePtr<AstNode> AllocaExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<AllocaExprNode> duplicated_node(make_ast_node<AllocaExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API AllocaExprNode::AllocaExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Alloca, self_allocator, document),
	  idx_comma_tokens(self_allocator) {
}
SLKC_API AllocaExprNode::AllocaExprNode(const AllocaExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context), idx_comma_tokens(allocator) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(count_expr = rhs.count_expr->duplicate<ExprNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(target_type = rhs.target_type->duplicate<TypeNameNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}
}
SLKC_API AllocaExprNode::~AllocaExprNode() {
}

SLKC_API AstNodePtr<AstNode> CastExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<CastExprNode> duplicated_node(make_ast_node<CastExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API CastExprNode::CastExprNode(
	peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Cast, self_allocator, document) {
}
SLKC_API CastExprNode::CastExprNode(const CastExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(target_type = rhs.target_type->duplicate<TypeNameNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(source = rhs.source->duplicate<ExprNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	nullable_token_index = rhs.nullable_token_index;

	as_keyword_token_index = rhs.as_keyword_token_index;

	succeeded_out = true;
}
SLKC_API CastExprNode::~CastExprNode() {
}

SLKC_API AstNodePtr<AstNode> MatchExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<MatchExprNode> duplicated_node(make_ast_node<MatchExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API MatchExprNode::MatchExprNode(
	peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Match, self_allocator, document),
	  cases(self_allocator) {
}
SLKC_API MatchExprNode::MatchExprNode(const MatchExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context), cases(allocator) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(return_type = rhs.return_type->duplicate<TypeNameNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(condition = rhs.condition->duplicate<ExprNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!cases.resize(rhs.cases.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < cases.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(cases.at(i).first = rhs.cases.at(i).first->duplicate<ExprNode>(allocator)))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}

		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(cases.at(i).second = rhs.cases.at(i).second->duplicate<ExprNode>(allocator)))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}
SLKC_API MatchExprNode::~MatchExprNode() {
}

SLKC_API AstNodePtr<AstNode> WrapperExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<WrapperExprNode> duplicated_node(make_ast_node<WrapperExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API WrapperExprNode::WrapperExprNode(
	peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Wrapper, self_allocator, document) {
}
SLKC_API WrapperExprNode::WrapperExprNode(const WrapperExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(target = rhs.target->duplicate<ExprNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}
SLKC_API WrapperExprNode::~WrapperExprNode() {
}

SLKC_API AstNodePtr<AstNode> RegIndexExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<RegIndexExprNode> duplicated_node(make_ast_node<RegIndexExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API RegIndexExprNode::RegIndexExprNode(
	peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, uint32_t reg, AstNodePtr<TypeNameNode> type)
	: ExprNode(ExprKind::RegIndex, self_allocator, document), reg(reg), type(type) {
}
SLKC_API RegIndexExprNode::RegIndexExprNode(const RegIndexExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context) {
	reg = rhs.reg;

	type = rhs.type;

	succeeded_out = true;
}
SLKC_API RegIndexExprNode::~RegIndexExprNode() {
}

SLKC_API AstNodePtr<AstNode> TypeNameExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<TypeNameExprNode> duplicated_node(make_ast_node<TypeNameExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API TypeNameExprNode::TypeNameExprNode(
	peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, AstNodePtr<TypeNameNode> type)
	: ExprNode(ExprKind::TypeName, self_allocator, document), type(type) {
}
SLKC_API TypeNameExprNode::TypeNameExprNode(const TypeNameExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context) {
	type = rhs.type;

	succeeded_out = true;
}
SLKC_API TypeNameExprNode::~TypeNameExprNode() {
}

SLKC_API AstNodePtr<AstNode> BadExprNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<BadExprNode> duplicated_node(make_ast_node<BadExprNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}
SLKC_API BadExprNode::BadExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	const AstNodePtr<ExprNode> &incomplete_expr)
	: ExprNode(ExprKind::Bad, self_allocator, document),
	  incomplete_expr(incomplete_expr) {
}
SLKC_API BadExprNode::BadExprNode(const BadExprNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: ExprNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(incomplete_expr = rhs.incomplete_expr->duplicate<ExprNode>(allocator)))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}
SLKC_API BadExprNode::~BadExprNode() {
}
