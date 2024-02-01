#include "../compiler.h"
#include <algorithm>

using namespace slake::slkc;

void Compiler::mergeContinuousRegAllocs(shared_ptr<CompiledFnNode> fn) {
	for (size_t i = 0; i < fn->body.size(); ++i) {
		if (fn->body[i].opcode == Opcode::REG) {
			size_t nTotalRegs = 0, nTotalIns = 0;

			size_t j = i;
			for (; j < fn->body.size(); ++j) {
				if (fn->body[j].opcode != Opcode::REG)
					break;

				++nTotalIns;

				assert(fn->body[j].operands[0]->getNodeType() == AST_EXPR);
				assert(static_pointer_cast<ExprNode>(fn->body[j].operands[0])->getExprType() == EXPR_U32);
				shared_ptr<U32LiteralExprNode> e = static_pointer_cast<U32LiteralExprNode>(fn->body[j].operands[0]);
				nTotalRegs += e->data;
			}

			for (auto k : fn->labels) {
				if (k.second > i) {
					k.second -= (nTotalIns - 1);
				}
			}

			for (auto k : fn->srcLocDescs) {
				if (k.offIns > i) {
					k.offIns -= (nTotalIns - 1);
				}
			}

			fn->body.erase(std::next(fn->body.begin(), i + 1), std::next(fn->body.begin(), j));
			fn->body[i].operands[0] = make_shared<U32LiteralExprNode>(fn->body[i].operands[0]->getLocation(), nTotalRegs);
		}
	}
}
