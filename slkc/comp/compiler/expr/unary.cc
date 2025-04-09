#include "../../compiler.h"

using namespace slkc;

std::optional<CompilationError> Compiler::compileUnaryExpr(
	FnCompileContext& compileContext,
	peff::SharedPtr<UnaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	peff::SharedPtr<TypeNameNode>& evaluatedTypeOut,
	bool evalTypeOnly) {
	switch (expr->unaryOp) {

	}

	return {};
}
