#include "astnode.h"
#include "document.h"

using namespace slkc;

SLAKE_API BaseAstNodeDuplicationTask::~BaseAstNodeDuplicationTask() {
}

SLKC_API AstNode::AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : _astNodeType(astNodeType), selfAllocator(selfAllocator), document(document.get()) {
	assert(document);
	document->clearDeferredDestructibleAstNodes();
}

SLAKE_API AstNode::AstNode(const AstNode &other, peff::Alloc *newAllocator, DuplicationContext &context) {
	other.document->clearDeferredDestructibleAstNodes();
	document = other.document;
	selfAllocator = newAllocator;
	_astNodeType = other._astNodeType;
	tokenRange = other.tokenRange;
}

SLKC_API AstNode::~AstNode() {
}

SLKC_API AstNodePtr<AstNode> AstNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	std::terminate();
}

#if SLKC_WITH_AST_DUMPING
SLKC_API wandjson::Value *AstNode::doDump(peff::Alloc *allocator, AstDumpingContext &context) const {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> value(wandjson::ObjectValue::alloc(allocator));

	if (!value)
		return nullptr;

	// TODO: Implement it.

	return value.release();
}

SLAKE_API wandjson::Value *AstNode::dump(peff::Alloc *allocator) const noexcept {
	AstDumpingContext context(allocator);

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> value(doDump(allocator, context));

	if (!value)
		return nullptr;

	while (context.dumpingTasks.size()) {
		auto tasks = std::move(context.dumpingTasks);

		for (auto &i : tasks) {
			switch (i.taskType) {
				case AstDumpingTaskType::ObjectMember: {
					auto &exData = std::get<ObjectMemberAstDumpingTaskExData>(i.exData);

					std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> newValue(exData.astNode->doDump(allocator, context));
					if (!newValue)
						return nullptr;

					peff::String name(allocator);

					if (!name.build(exData.name))
						return nullptr;

					if (!exData.objectValue->insert(std::move(name), newValue.get()))
						return nullptr;

					newValue.release();

					break;
				}
				case AstDumpingTaskType::ArrayInsertion: {
					auto &exData = std::get<ArrayInsertionAstDumpingTaskExData>(i.exData);

					std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> newValue(exData.astNode->doDump(allocator, context));
					if (!newValue)
						return nullptr;

					if (!exData.arrayValue->pushBack(newValue.get()))
						return nullptr;

					newValue.release();

					break;
				}
				default:
					SLAKE_UNREACHABLE();
			}
		}

		context.dumpingTasks = peff::List<AstDumpingTask>(allocator);
	}

	return value.release();
}
#endif

SLKC_API void slkc::addAstNodeToDestructibleList(AstNode *astNode, AstNodeDestructor _destructor) {
	astNode->_nextDestructible = astNode->document->destructibleAstNodeList;
	astNode->_destructor = _destructor;
	astNode->document->destructibleAstNodeList = astNode;
}
