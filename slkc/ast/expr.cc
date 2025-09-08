#include "expr.h"

using namespace slkc;

SLKC_API ExprNode::ExprNode(ExprKind exprKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::TypeName, selfAllocator, document), exprKind(exprKind) {
}

SLKC_API ExprNode::ExprNode(const ExprNode &rhs, peff::Alloc *allocator) : AstNode(rhs, allocator), exprKind(rhs.exprKind) {
}

SLKC_API ExprNode::~ExprNode() {
}

SLKC_API AstNodePtr<AstNode> UnaryExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;

	AstNodePtr<UnaryExprNode> duplicatedNode(makeAstNode<UnaryExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API UnaryExprNode::UnaryExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Unary, selfAllocator, document) {
}

SLKC_API UnaryExprNode::UnaryExprNode(const UnaryExprNode &rhs, peff::Alloc *selfAllocator, bool &succeededOut) : ExprNode(rhs, selfAllocator), unaryOp(rhs.unaryOp) {
	if (!(operand = rhs.operand->duplicate<ExprNode>(selfAllocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API UnaryExprNode::~UnaryExprNode() {
}

SLKC_API const char *slkc::getBinaryOperatorOverloadingName(BinaryOp op) {
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

SLKC_API AstNodePtr<AstNode> BinaryExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<BinaryExprNode> duplicatedNode(makeAstNode<BinaryExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API BinaryExprNode::BinaryExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Binary, selfAllocator, document) {
}

SLKC_API BinaryExprNode::BinaryExprNode(const BinaryExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator), binaryOp(rhs.binaryOp) {
	if (!(this->lhs = rhs.lhs->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}
	if (!(this->rhs = rhs.rhs->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API BinaryExprNode::~BinaryExprNode() {
}

SLKC_API AstNodePtr<AstNode> TernaryExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<TernaryExprNode> duplicatedNode(makeAstNode<TernaryExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API TernaryExprNode::TernaryExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Ternary, selfAllocator, document),
	  cond(cond),
	  lhs(lhs),
	  rhs(rhs) {
}

SLKC_API TernaryExprNode::TernaryExprNode(const TernaryExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator) {
	if (!(this->cond = rhs.cond->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}
	if (!(this->lhs = rhs.lhs->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}
	if (!(this->rhs = rhs.rhs->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}

SLKC_API TernaryExprNode::~TernaryExprNode() {
}

SLKC_API AstNodePtr<AstNode> LooseIdExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<LooseIdExprNode> duplicatedNode(makeAstNode<LooseIdExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API LooseIdExprNode::LooseIdExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	peff::String &&id)
	: ExprNode(ExprKind::IdRef, selfAllocator, document),
	  id(std::move(id)) {
}
SLKC_API LooseIdExprNode::LooseIdExprNode(const LooseIdExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator), id(allocator) {
	if (!(id.build(rhs.id))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}
SLKC_API LooseIdExprNode::~LooseIdExprNode() {
}

SLKC_API AstNodePtr<AstNode> IdRefExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<IdRefExprNode> duplicatedNode(makeAstNode<IdRefExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API IdRefExprNode::IdRefExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	IdRefPtr &&idRefPtr)
	: ExprNode(ExprKind::IdRef, selfAllocator, document),
	  idRefPtr(std::move(idRefPtr)) {
}
SLKC_API IdRefExprNode::IdRefExprNode(const IdRefExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator) {
	if (!(idRefPtr = duplicateIdRef(allocator, rhs.idRefPtr.get()))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}
SLKC_API IdRefExprNode::~IdRefExprNode() {
}

SLKC_API AstNodePtr<AstNode> HeadedIdRefExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<HeadedIdRefExprNode> duplicatedNode(makeAstNode<HeadedIdRefExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API HeadedIdRefExprNode::HeadedIdRefExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	const AstNodePtr<ExprNode> &head,
	IdRefPtr &&idRefPtr)
	: ExprNode(ExprKind::HeadedIdRef, selfAllocator, document),
	  idRefPtr(std::move(idRefPtr)) {
}
SLKC_API HeadedIdRefExprNode::HeadedIdRefExprNode(const HeadedIdRefExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator) {
	if (!(head = rhs.head->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}
	if (!(idRefPtr = duplicateIdRef(allocator, rhs.idRefPtr.get()))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}
SLKC_API HeadedIdRefExprNode::~HeadedIdRefExprNode() {
}

SLKC_API AstNodePtr<AstNode> I8LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I8LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<I8LiteralExprNode, AstNodeControlBlock<I8LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I8LiteralExprNode::I8LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	int8_t data)
	: ExprNode(ExprKind::I8, selfAllocator, document),
	  data(data) {
}
SLKC_API I8LiteralExprNode::I8LiteralExprNode(const I8LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API I8LiteralExprNode::~I8LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> I16LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I16LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<I16LiteralExprNode, AstNodeControlBlock<I16LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I16LiteralExprNode::I16LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	int16_t data)
	: ExprNode(ExprKind::I16, selfAllocator, document),
	  data(data) {
}
SLKC_API I16LiteralExprNode::I16LiteralExprNode(const I16LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API I16LiteralExprNode::~I16LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> I32LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I32LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<I32LiteralExprNode, AstNodeControlBlock<I32LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I32LiteralExprNode::I32LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	int32_t data)
	: ExprNode(ExprKind::I32, selfAllocator, document),
	  data(data) {
}
SLKC_API I32LiteralExprNode::I32LiteralExprNode(const I32LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API I32LiteralExprNode::~I32LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> I64LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<I64LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<I64LiteralExprNode, AstNodeControlBlock<I64LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API I64LiteralExprNode::I64LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	int64_t data)
	: ExprNode(ExprKind::I64, selfAllocator, document),
	  data(data) {
}
SLKC_API I64LiteralExprNode::I64LiteralExprNode(const I64LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API I64LiteralExprNode::~I64LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U8LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U8LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<U8LiteralExprNode, AstNodeControlBlock<U8LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U8LiteralExprNode::U8LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	uint8_t data)
	: ExprNode(ExprKind::U8, selfAllocator, document),
	  data(data) {
}
SLKC_API U8LiteralExprNode::U8LiteralExprNode(const U8LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API U8LiteralExprNode::~U8LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U16LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U16LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<U16LiteralExprNode, AstNodeControlBlock<U16LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U16LiteralExprNode::U16LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	uint16_t data)
	: ExprNode(ExprKind::U16, selfAllocator, document),
	  data(data) {
}
SLKC_API U16LiteralExprNode::U16LiteralExprNode(const U16LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API U16LiteralExprNode::~U16LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U32LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U32LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<U32LiteralExprNode, AstNodeControlBlock<U32LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API U32LiteralExprNode::U32LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	uint32_t data)
	: ExprNode(ExprKind::U32, selfAllocator, document),
	  data(data) {
}
SLKC_API U32LiteralExprNode::U32LiteralExprNode(const U32LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API U32LiteralExprNode::~U32LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> U64LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<U64LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<U64LiteralExprNode, AstNodeControlBlock<U64LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API U64LiteralExprNode::U64LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	uint64_t data)
	: ExprNode(ExprKind::U64, selfAllocator, document),
	  data(data) {
}
SLKC_API U64LiteralExprNode::U64LiteralExprNode(const U64LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API U64LiteralExprNode::~U64LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> F32LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<F32LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<F32LiteralExprNode, AstNodeControlBlock<F32LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API F32LiteralExprNode::F32LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	float data)
	: ExprNode(ExprKind::F32, selfAllocator, document),
	  data(data) {
}
SLKC_API F32LiteralExprNode::F32LiteralExprNode(const F32LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API F32LiteralExprNode::~F32LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> F64LiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	peff::SharedPtr<F64LiteralExprNode> duplicatedNode(peff::makeSharedWithControlBlock<F64LiteralExprNode, AstNodeControlBlock<F64LiteralExprNode>>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}

SLKC_API F64LiteralExprNode::F64LiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	double data)
	: ExprNode(ExprKind::F64, selfAllocator, document),
	  data(data) {
}
SLKC_API F64LiteralExprNode::F64LiteralExprNode(const F64LiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API F64LiteralExprNode::~F64LiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> BoolLiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	AstNodePtr<BoolLiteralExprNode> duplicatedNode(makeAstNode<BoolLiteralExprNode>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API BoolLiteralExprNode::BoolLiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	bool data)
	: ExprNode(ExprKind::Bool, selfAllocator, document),
	  data(data) {
}
SLKC_API BoolLiteralExprNode::BoolLiteralExprNode(const BoolLiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator), data(rhs.data) {
}
SLKC_API BoolLiteralExprNode::~BoolLiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> StringLiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<StringLiteralExprNode> duplicatedNode(makeAstNode<StringLiteralExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API StringLiteralExprNode::StringLiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	peff::String &&data)
	: ExprNode(ExprKind::String, selfAllocator, document),
	  data(std::move(data)) {
}
SLKC_API StringLiteralExprNode::StringLiteralExprNode(const StringLiteralExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator), data(allocator) {
	if (!data.build(rhs.data)) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}
SLKC_API StringLiteralExprNode::~StringLiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> NullLiteralExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	AstNodePtr<NullLiteralExprNode> duplicatedNode(makeAstNode<NullLiteralExprNode>(newAllocator, *this, newAllocator));
	if (!duplicatedNode) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API NullLiteralExprNode::NullLiteralExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Null, selfAllocator, document) {
}
SLKC_API NullLiteralExprNode::NullLiteralExprNode(const NullLiteralExprNode &rhs, peff::Alloc *allocator) : ExprNode(rhs, allocator) {
}
SLKC_API NullLiteralExprNode::~NullLiteralExprNode() {
}

SLKC_API AstNodePtr<AstNode> InitializerListExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<InitializerListExprNode> duplicatedNode(makeAstNode<InitializerListExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API InitializerListExprNode::InitializerListExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::InitializerList, selfAllocator, document),
	  elements(selfAllocator) {
}
SLKC_API InitializerListExprNode::InitializerListExprNode(const InitializerListExprNode &rhs, peff::Alloc *allocator, bool &succeededOut) : ExprNode(rhs, allocator), elements(allocator) {
	if (!elements.resize(rhs.elements.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < elements.size(); ++i) {
		if (!(elements.at(i) = rhs.elements.at(i)->duplicate<ExprNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}
SLKC_API InitializerListExprNode::~InitializerListExprNode() {
}

SLKC_API AstNodePtr<AstNode> CallExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<CallExprNode> duplicatedNode(makeAstNode<CallExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API CallExprNode::CallExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	const AstNodePtr<ExprNode> &target,
	peff::DynArray<AstNodePtr<ExprNode>> &&args)
	: ExprNode(ExprKind::Call, selfAllocator, document),
	  target(target),
	  args(std::move(args)),
	  idxCommaTokens(selfAllocator) {
}
SLKC_API CallExprNode::CallExprNode(const CallExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator), args(allocator), idxCommaTokens(allocator) {
	if (rhs.target && !(target = rhs.target->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (rhs.withObject && !(withObject = rhs.withObject->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!args.resize(rhs.args.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < args.size(); ++i) {
		if (!(args.at(i) = rhs.args.at(i)->duplicate<ExprNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}
}
SLKC_API CallExprNode::~CallExprNode() {
}

SLKC_API AstNodePtr<AstNode> NewExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<NewExprNode> duplicatedNode(makeAstNode<NewExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API NewExprNode::NewExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::New, selfAllocator, document),
	  args(selfAllocator),
	  idxCommaTokens(selfAllocator) {
}
SLKC_API NewExprNode::NewExprNode(const NewExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator), args(allocator), idxCommaTokens(allocator) {
	if (!(targetType = rhs.targetType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!args.resize(rhs.args.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < args.size(); ++i) {
		if (!(args.at(i) = rhs.args.at(i)->duplicate<ExprNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}
}
SLKC_API NewExprNode::~NewExprNode() {
}

SLKC_API AstNodePtr<AstNode> AllocaExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<AllocaExprNode> duplicatedNode(makeAstNode<AllocaExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API AllocaExprNode::AllocaExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Alloca, selfAllocator, document),
	  idxCommaTokens(selfAllocator) {
}
SLKC_API AllocaExprNode::AllocaExprNode(const AllocaExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator), idxCommaTokens(allocator) {
	if (!(countExpr = rhs.countExpr->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(targetType = rhs.targetType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}
}
SLKC_API AllocaExprNode::~AllocaExprNode() {
}

SLKC_API AstNodePtr<AstNode> CastExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<CastExprNode> duplicatedNode(makeAstNode<CastExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API CastExprNode::CastExprNode(
	peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Cast, selfAllocator, document) {
}
SLKC_API CastExprNode::CastExprNode(const CastExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator) {
	if (!(targetType = rhs.targetType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(source = rhs.source->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	asKeywordTokenIndex = rhs.asKeywordTokenIndex;

	succeededOut = true;
}
SLKC_API CastExprNode::~CastExprNode() {
}

SLKC_API AstNodePtr<AstNode> MatchExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<MatchExprNode> duplicatedNode(makeAstNode<MatchExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API MatchExprNode::MatchExprNode(
	peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document)
	: ExprNode(ExprKind::Match, selfAllocator, document),
	  cases(selfAllocator) {
}
SLKC_API MatchExprNode::MatchExprNode(const MatchExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator), cases(allocator) {
	if (!(returnType = rhs.returnType->duplicate<TypeNameNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!(condition = rhs.condition->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	if (!cases.resize(rhs.cases.size())) {
		succeededOut = false;
		return;
	}

	for (size_t i = 0; i < cases.size(); ++i) {
		if (!(cases.at(i).first = rhs.cases.at(i).first->duplicate<ExprNode>(allocator))) {
			succeededOut = false;
			return;
		}

		if (!(cases.at(i).second = rhs.cases.at(i).second->duplicate<ExprNode>(allocator))) {
			succeededOut = false;
			return;
		}
	}

	succeededOut = true;
}
SLKC_API MatchExprNode::~MatchExprNode() {
}

SLKC_API AstNodePtr<AstNode> WrapperExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<WrapperExprNode> duplicatedNode(makeAstNode<WrapperExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API WrapperExprNode::WrapperExprNode(
	peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &target)
	: ExprNode(ExprKind::Wrapper, selfAllocator, document),
	  target(target) {
}
SLKC_API WrapperExprNode::WrapperExprNode(const WrapperExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator) {
	if (!(target = rhs.target->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}
SLKC_API WrapperExprNode::~WrapperExprNode() {
}

SLKC_API AstNodePtr<AstNode> RegRefExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<RegRefExprNode> duplicatedNode(makeAstNode<RegRefExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API RegRefExprNode::RegRefExprNode(
	peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, uint32_t reg, AstNodePtr<TypeNameNode> type)
	: ExprNode(ExprKind::RegRef, selfAllocator, document), reg(reg), type(type) {
}
SLKC_API RegRefExprNode::RegRefExprNode(const RegRefExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator) {
	reg = rhs.reg;

	type = rhs.type;

	succeededOut = true;
}
SLKC_API RegRefExprNode::~RegRefExprNode() {
}

SLKC_API AstNodePtr<AstNode> BadExprNode::doDuplicate(peff::Alloc *newAllocator) const {
	bool succeeded = false;
	AstNodePtr<BadExprNode> duplicatedNode(makeAstNode<BadExprNode>(newAllocator, *this, newAllocator, succeeded));
	if ((!duplicatedNode) || (!succeeded)) {
		return {};
	}

	return duplicatedNode.castTo<AstNode>();
}
SLKC_API BadExprNode::BadExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	const AstNodePtr<ExprNode> &incompleteExpr)
	: ExprNode(ExprKind::Bad, selfAllocator, document),
	  incompleteExpr(incompleteExpr) {
}
SLKC_API BadExprNode::BadExprNode(const BadExprNode &rhs, peff::Alloc *allocator, bool &succeededOut)
	: ExprNode(rhs, allocator) {
	if (!(incompleteExpr = rhs.incompleteExpr->duplicate<ExprNode>(allocator))) {
		succeededOut = false;
		return;
	}

	succeededOut = true;
}
SLKC_API BadExprNode::~BadExprNode() {
}
