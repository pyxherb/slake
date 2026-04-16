#include "stmt.h"

using namespace slkc;

SLKC_API StmtNode::StmtNode(StmtKind stmt_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::Stmt, self_allocator, document), stmt_kind(stmt_kind) {
}

SLKC_API StmtNode::StmtNode(const StmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : AstNode(rhs, allocator, context), stmt_kind(rhs.stmt_kind) {
}

SLKC_API StmtNode::~StmtNode() {
}

SLKC_API AstNodePtr<AstNode> ExprStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ExprStmtNode> duplicated_node(make_ast_node<ExprStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ExprStmtNode::ExprStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Expr, self_allocator, document) {
}

SLKC_API ExprStmtNode::ExprStmtNode(const ExprStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(expr = rhs.expr->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ExprStmtNode::~ExprStmtNode() {
}

SLKC_API VarDefEntry::VarDefEntry(peff::Alloc *self_allocator) : self_allocator(self_allocator), name(self_allocator), attributes(self_allocator) {
}
SLKC_API VarDefEntry::~VarDefEntry() {
}
SLKC_API void VarDefEntry::dealloc() noexcept {
	peff::destroy_and_release<VarDefEntry>(self_allocator.get(), this, ASTNODE_ALIGNMENT);
}

SLKC_API VarDefEntryPtr slkc::duplicate_var_def_entry(VarDefEntry *var_def_entry, peff::Alloc *allocator) {
	VarDefEntryPtr ptr(peff::alloc_and_construct<VarDefEntry>(allocator, ASTNODE_ALIGNMENT, allocator));

	if (!ptr->name.build(var_def_entry->name)) {
		return {};
	}

	if (var_def_entry->type && !(ptr->type = var_def_entry->type->duplicate<TypeNameNode>(allocator))) {
		return {};
	}

	if (var_def_entry->initial_value && !(ptr->initial_value = var_def_entry->initial_value->duplicate<ExprNode>(allocator))) {
		return {};
	}

	ptr->idx_name_token = var_def_entry->idx_name_token;

	return ptr;
}

SLKC_API AstNodePtr<AstNode> VarDefStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<VarDefStmtNode> duplicated_node(make_ast_node<VarDefStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API VarDefStmtNode::VarDefStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, peff::DynArray<VarDefEntryPtr> &&var_def_entries) : StmtNode(StmtKind::VarDef, self_allocator, document), var_def_entries(std::move(var_def_entries)) {
}

SLKC_API VarDefStmtNode::VarDefStmtNode(const VarDefStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), var_def_entries(allocator) {
	if (!(var_def_entries.resize(rhs.var_def_entries.size()))) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < var_def_entries.size(); ++i) {
		if (!(var_def_entries.at(i) = duplicate_var_def_entry(rhs.var_def_entries.at(i).get(), allocator))) {
			succeeded_out = false;
			return;
		}
	}

	access_modifier = rhs.access_modifier;

	succeeded_out = true;
}

SLKC_API VarDefStmtNode::~VarDefStmtNode() {
}

SLKC_API AstNodePtr<AstNode> BreakStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<BreakStmtNode> duplicated_node(make_ast_node<BreakStmtNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API BreakStmtNode::BreakStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Break, self_allocator, document) {
}

SLKC_API BreakStmtNode::BreakStmtNode(const BreakStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : StmtNode(rhs, allocator, context) {
}

SLKC_API BreakStmtNode::~BreakStmtNode() {
}

SLKC_API AstNodePtr<AstNode> ContinueStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<ContinueStmtNode> duplicated_node(make_ast_node<ContinueStmtNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ContinueStmtNode::ContinueStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Continue, self_allocator, document) {
}

SLKC_API ContinueStmtNode::ContinueStmtNode(const ContinueStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context) : StmtNode(rhs, allocator, context) {
}

SLKC_API ContinueStmtNode::~ContinueStmtNode() {
}

SLKC_API AstNodePtr<AstNode> ForStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ForStmtNode> duplicated_node(make_ast_node<ForStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ForStmtNode::ForStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::For, self_allocator, document), var_def_entries(self_allocator) {
}

SLKC_API ForStmtNode::ForStmtNode(const ForStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), var_def_entries(allocator) {
	if (!(var_def_entries.resize(rhs.var_def_entries.size()))) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < var_def_entries.size(); ++i) {
		if (!(var_def_entries.at(i) = duplicate_var_def_entry(rhs.var_def_entries.at(i).get(), allocator))) {
			succeeded_out = false;
			return;
		}
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(cond = rhs.cond->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(step = rhs.step->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(body = rhs.body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ForStmtNode::~ForStmtNode() {
}

SLKC_API AstNodePtr<AstNode> ForEachStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ForEachStmtNode> duplicated_node(make_ast_node<ForEachStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ForEachStmtNode::ForEachStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, peff::String &&var_name, const AstNodePtr<ExprNode> &cond, const AstNodePtr<StmtNode> &body) : StmtNode(StmtKind::ForEach, self_allocator, document), var_name(std::move(var_name)), cond(cond), body(body) {
}

SLKC_API ForEachStmtNode::ForEachStmtNode(const ForEachStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), var_name(allocator) {
	if (!var_name.build(rhs.var_name)) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(cond = rhs.cond->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(body = rhs.body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ForEachStmtNode::~ForEachStmtNode() {
}

SLKC_API AstNodePtr<AstNode> WhileStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<WhileStmtNode> duplicated_node(make_ast_node<WhileStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API WhileStmtNode::WhileStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::While, self_allocator, document) {
}

SLKC_API WhileStmtNode::WhileStmtNode(const WhileStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(cond = rhs.cond->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(body = rhs.body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API WhileStmtNode::~WhileStmtNode() {
}

SLKC_API AstNodePtr<AstNode> DoWhileStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<DoWhileStmtNode> duplicated_node(make_ast_node<DoWhileStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API DoWhileStmtNode::DoWhileStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::While, self_allocator, document) {
}

SLKC_API DoWhileStmtNode::DoWhileStmtNode(const DoWhileStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(cond = rhs.cond->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(body = rhs.body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API DoWhileStmtNode::~DoWhileStmtNode() {
}

SLKC_API AstNodePtr<AstNode> ReturnStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ReturnStmtNode> duplicated_node(make_ast_node<ReturnStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ReturnStmtNode::ReturnStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value) : StmtNode(StmtKind::Return, self_allocator, document), value(value) {
}

SLKC_API ReturnStmtNode::ReturnStmtNode(const ReturnStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(value = rhs.value->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ReturnStmtNode::~ReturnStmtNode() {
}

SLKC_API AstNodePtr<AstNode> YieldStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<YieldStmtNode> duplicated_node(make_ast_node<YieldStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API YieldStmtNode::YieldStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<ExprNode> &value) : StmtNode(StmtKind::Yield, self_allocator, document), value(value) {
}

SLKC_API YieldStmtNode::YieldStmtNode(const YieldStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(value = rhs.value->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API YieldStmtNode::~YieldStmtNode() {
}

SLKC_API AstNodePtr<AstNode> IfStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<IfStmtNode> duplicated_node(make_ast_node<IfStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API IfStmtNode::IfStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::If, self_allocator, document) {
}

SLKC_API IfStmtNode::IfStmtNode(const IfStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(cond = rhs.cond->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(true_body = rhs.true_body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(false_body = rhs.false_body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API IfStmtNode::~IfStmtNode() {
}

SLKC_API WithConstraintEntry::WithConstraintEntry(peff::Alloc *self_allocator) : self_allocator(self_allocator), generic_param_name(self_allocator) {}
SLKC_API WithConstraintEntry::~WithConstraintEntry() {}
SLKC_API void WithConstraintEntry::dealloc() noexcept {
	peff::destroy_and_release<WithConstraintEntry>(self_allocator.get(), this, alignof(WithConstraintEntry));
}

WithConstraintEntryPtr slkc::duplicate_with_constraint_entry(peff::Alloc *allocator, const WithConstraintEntry *constraint) {
	WithConstraintEntryPtr ptr(peff::alloc_and_construct<WithConstraintEntry>(allocator, alignof(WithConstraintEntry), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (!ptr->generic_param_name.build(constraint->generic_param_name)) {
		return nullptr;
	}

	if (!(ptr->constraint = duplicate_generic_constraint(allocator, constraint->constraint.get()))) {
		return nullptr;
	}

	return ptr;
}

SLKC_API AstNodePtr<AstNode> WithStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<WithStmtNode> duplicated_node(make_ast_node<WithStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API WithStmtNode::WithStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::With, self_allocator, document), constraints(self_allocator) {
}

SLKC_API WithStmtNode::WithStmtNode(const WithStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), constraints(allocator) {
	if (!constraints.resize(rhs.constraints.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < rhs.constraints.size(); ++i) {
		if (!(constraints.at(i) = duplicate_with_constraint_entry(allocator, rhs.constraints.at(i).get()))) {
			succeeded_out = false;
			return;
		}
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(true_body = rhs.true_body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(false_body = rhs.false_body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API WithStmtNode::~WithStmtNode() {
}

SLKC_API AstNodePtr<AstNode> CaseLabelStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<CaseLabelStmtNode> duplicated_node(make_ast_node<CaseLabelStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API CaseLabelStmtNode::CaseLabelStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::CaseLabel, self_allocator, document) {
}

SLKC_API CaseLabelStmtNode::CaseLabelStmtNode(const CaseLabelStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(condition = rhs.condition->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API CaseLabelStmtNode::~CaseLabelStmtNode() {
}

SLKC_API AstNodePtr<AstNode> SwitchStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<SwitchStmtNode> duplicated_node(make_ast_node<SwitchStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API SwitchStmtNode::SwitchStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Switch, self_allocator, document), case_offsets(self_allocator), body(self_allocator) {
}

SLKC_API SwitchStmtNode::SwitchStmtNode(const SwitchStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), case_offsets(allocator), body(allocator) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(condition = rhs.condition->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!(body.resize(rhs.body.size()))) {
		succeeded_out = false;
		return;
	}

	if (!case_offsets.resize(rhs.case_offsets.size())) {
		succeeded_out = false;
		return;
	}

	memcpy(case_offsets.data(), rhs.case_offsets.data(), sizeof(size_t) * case_offsets.size());

	for (size_t i = 0; i < body.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(body.at(i) = rhs.body.at(i)->do_duplicate(allocator, context).cast_to<StmtNode>()))
					return true;
				return false;
			})) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}

SLKC_API SwitchStmtNode::~SwitchStmtNode() {
}

SLKC_API AstNodePtr<AstNode> LabelStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<LabelStmtNode> duplicated_node(make_ast_node<LabelStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API LabelStmtNode::LabelStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::Label, self_allocator, document), name(self_allocator) {
}

SLKC_API LabelStmtNode::LabelStmtNode(const LabelStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), name(allocator) {
	if (!name.build(rhs.name)) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API LabelStmtNode::~LabelStmtNode() {
}

SLKC_API AstNodePtr<AstNode> CodeBlockStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<CodeBlockStmtNode> duplicated_node(make_ast_node<CodeBlockStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API CodeBlockStmtNode::CodeBlockStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : StmtNode(StmtKind::CodeBlock, self_allocator, document), body(self_allocator) {
}

SLKC_API CodeBlockStmtNode::CodeBlockStmtNode(const CodeBlockStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context), body(allocator) {
	if (!(body.resize(rhs.body.size()))) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < body.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(body.at(i) = rhs.body.at(i)->do_duplicate(allocator, context).cast_to<StmtNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}

SLKC_API CodeBlockStmtNode::~CodeBlockStmtNode() {
}

SLKC_API AstNodePtr<AstNode> BadStmtNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded;
	AstNodePtr<BadStmtNode> duplicated_node(make_ast_node<BadStmtNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API BadStmtNode::BadStmtNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<StmtNode> &body) : StmtNode(StmtKind::Bad, self_allocator, document), body(body) {
}

SLKC_API BadStmtNode::BadStmtNode(const BadStmtNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : StmtNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(body = rhs.body->do_duplicate(allocator, context).cast_to<StmtNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API BadStmtNode::~BadStmtNode() {
}
