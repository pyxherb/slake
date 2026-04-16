#include "astnode.h"
#include "document.h"

using namespace slkc;

SLAKE_API BaseAstNodeDuplicationTask::~BaseAstNodeDuplicationTask() {
}

SLKC_API AstNode::AstNode(AstNodeType ast_node_type, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : _ast_node_type(ast_node_type), self_allocator(self_allocator), document(document.get()) {
	assert(document);
	document->clear_deferred_destructible_ast_nodes();
}

SLAKE_API AstNode::AstNode(const AstNode &other, peff::Alloc *new_allocator, DuplicationContext &context): self_allocator(new_allocator) {
	other.document->clear_deferred_destructible_ast_nodes();
	document = other.document;
	_ast_node_type = other._ast_node_type;
	token_range = other.token_range;
}

SLKC_API AstNode::~AstNode() {
}

SLKC_API AstNodePtr<AstNode> AstNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	std::terminate();
}

#if SLKC_WITH_AST_DUMPING
SLKC_API wandjson::Value *AstNode::do_dump(peff::Alloc *allocator, AstDumpingContext &context) const {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> value(wandjson::ObjectValue::alloc(allocator));

	if (!value)
		return nullptr;

	// TODO: Implement it.

	return value.release();
}

SLAKE_API wandjson::Value *AstNode::dump(peff::Alloc *allocator) const noexcept {
	AstDumpingContext context(allocator);

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> value(do_dump(allocator, context));

	if (!value)
		return nullptr;

	while (context.dumping_tasks.size()) {
		auto tasks = std::move(context.dumping_tasks);

		for (auto &i : tasks) {
			switch (i.task_type) {
				case AstDumpingTaskType::ObjectMember: {
					auto &ex_data = std::get<ObjectMemberAstDumpingTaskExData>(i.ex_data);

					std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> new_value(ex_data.ast_node->do_dump(allocator, context));
					if (!new_value)
						return nullptr;

					peff::String name(allocator);

					if (!name.build(ex_data.name))
						return nullptr;

					if (!ex_data.object_value->insert(std::move(name), new_value.get()))
						return nullptr;

					new_value.release();

					break;
				}
				case AstDumpingTaskType::ArrayInsertion: {
					auto &ex_data = std::get<ArrayInsertionAstDumpingTaskExData>(i.ex_data);

					std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> new_value(ex_data.ast_node->do_dump(allocator, context));
					if (!new_value)
						return nullptr;

					if (!ex_data.array_value->push_back(new_value.get()))
						return nullptr;

					new_value.release();

					break;
				}
				default:
					SLAKE_UNREACHABLE();
			}
		}

		context.dumping_tasks = peff::List<AstDumpingTask>(allocator);
	}

	return value.release();
}
#endif

SLKC_API void slkc::add_ast_node_to_destructible_list(AstNode *ast_node, AstNodeDestructor _destructor) {
	ast_node->_next_destructible = ast_node->document->destructible_ast_node_list;
	ast_node->_destructor = _destructor;
	ast_node->document->destructible_ast_node_list = ast_node;
}
