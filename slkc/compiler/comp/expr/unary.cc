#include "../../compiler.h"

using namespace slake::slkc;

std::map<UnaryOp, Compiler::UnaryOpRegistry> Compiler::_unaryOpRegs = {
	{ UnaryOp::LNot, { slake::Opcode::LNOT, false, false } },
	{ UnaryOp::Not, { slake::Opcode::NOT, false, false } },
	{ UnaryOp::Neg, { slake::Opcode::NEG, false, false } }
};

void slake::slkc::Compiler::compileUnaryOpExpr(std::shared_ptr<UnaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType) {
	uint32_t resultRegIndex = allocReg(),
			 lhsRegIndex = allocReg();

	if (!lhsType)
		throw FatalCompilationError(
			Message(
				e->x->sourceLocation,
				MessageType::Error,
				"Error deducing type of the operand"));

	auto &opReg = _unaryOpRegs.at(e->op);

	std::shared_ptr<TypeNameNode> resultType;

	switch (lhsType->getTypeId()) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
			switch (e->op) {
				case UnaryOp::LNot:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						{ std::make_shared<RegRefNode>(resultRegIndex),
							std::make_shared<RegRefNode>(lhsRegIndex, true) });

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				case UnaryOp::Not:
				case UnaryOp::Neg:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						{ std::make_shared<RegRefNode>(resultRegIndex),
							std::make_shared<RegRefNode>(lhsRegIndex, true) });

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.lvalueResult;
					break;
				default:
					throw FatalCompilationError(
						Message(
							e->sourceLocation,
							MessageType::Error,
							"No matching operator"));
			}
			break;
		case TypeId::F32:
		case TypeId::F64:
			switch (e->op) {
				case UnaryOp::LNot:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						{ std::make_shared<RegRefNode>(resultRegIndex),
							std::make_shared<RegRefNode>(lhsRegIndex, true) });

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				case UnaryOp::Neg:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						{ std::make_shared<RegRefNode>(resultRegIndex),
							std::make_shared<RegRefNode>(lhsRegIndex, true) });

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.lvalueResult;
					break;
				default:
					throw FatalCompilationError(
						Message(
							e->sourceLocation,
							MessageType::Error,
							"No matching operator"));
			}
			break;
		case TypeId::Bool:
			switch (e->op) {
				case UnaryOp::LNot:
				case UnaryOp::Not:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						{ std::make_shared<RegRefNode>(resultRegIndex),
							std::make_shared<RegRefNode>(lhsRegIndex, true) });

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				default:
					throw FatalCompilationError(
						Message(
							e->sourceLocation,
							MessageType::Error,
							"No matching operator"));
			}

			break;
		case TypeId::Custom: {
			auto node = resolveCustomTypeName(std::static_pointer_cast<CustomTypeNameNode>(lhsType).get());

			auto determineOverloading = [this, e, resultRegIndex, &resultType](std::shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> bool {
				if (auto it = n->scope->members.find("operator" + std::to_string(e->op));
					it != n->scope->members.end()) {
					assert(it->second->getNodeType() == NodeType::Fn);
					std::shared_ptr<FnNode> operatorNode = std::static_pointer_cast<FnNode>(it->second);
					std::shared_ptr<FnOverloadingNode> overloading;

					{
						auto overloadings = argDependentLookup(operatorNode.get(), {}, {});
						if (overloadings.size() != 1)
							return false;
						overloading = overloadings[0];
					}

#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(e->idxOpToken, [&overloading](TokenInfo &tokenInfo) {
						tokenInfo.semanticInfo.correspondingMember = overloading;
					});
#endif

					compileExpr(
						e->x,
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					IdRef operatorName = { overloading->getName() };

					uint32_t tmpRegIndex = allocReg();

					if (overloading->isVirtual)
						_insertIns(
							Opcode::RLOAD,
							{ std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(lhsRegIndex, true),
								std::make_shared<IdRefExprNode>(operatorName) });
					else {
						IdRef fullName = getFullName(overloading.get());
						_insertIns(
							Opcode::LOAD,
							{ std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<IdRefExprNode>(fullName) });
					}
					_insertIns(
						Opcode::MCALL,
						{ std::make_shared<RegRefNode>(tmpRegIndex, true),
							std::make_shared<RegRefNode>(lhsRegIndex, true) });

					_insertIns(Opcode::LRET, { std::make_shared<RegRefNode>(resultRegIndex) });

					resultType = overloading->returnType;

					return true;
				}
				return false;
			};

			switch (node->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface: {
					uint32_t lhsRegIndex = allocReg(1);

					std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

					if (!determineOverloading(n, lhsRegIndex))
						throw FatalCompilationError(
							Message(
								e->sourceLocation,
								MessageType::Error,
								"No matching operator"));

					break;
				}
				case NodeType::GenericParam: {
					uint32_t lhsRegIndex = allocReg(1);

					std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

					std::shared_ptr<AstNode> curMember;

					curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

					if (curMember->getNodeType() != NodeType::Class)
						throw FatalCompilationError(
							Message(
								n->baseType->sourceLocation,
								MessageType::Error,
								"Must be a class"));

					if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
						break;

					for (auto i : n->interfaceTypes) {
						curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

						if (curMember->getNodeType() != NodeType::Interface)
							throw FatalCompilationError(
								Message(
									n->baseType->sourceLocation,
									MessageType::Error,
									"Must be an interface"));

						if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
							break;
					}

					throw FatalCompilationError(
						Message(
							e->sourceLocation,
							MessageType::Error,
							"No matching operator"));
					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							e->sourceLocation,
							MessageType::Error,
							"No matching operator"));
			}
			break;
		}
		default:
			throw FatalCompilationError(
				Message(
					e->sourceLocation,
					MessageType::Error,
					"No matching operator"));
	}

	assert(resultType);

	if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue) {
		if (!isLValueType(resultType))
			throw FatalCompilationError(
				Message(
					e->sourceLocation,
					MessageType::Error,
					"Expecting a lvalue expression"));
	} else {
		if (isLValueType(resultType)) {
			_insertIns(
				Opcode::LVALUE,
				{ std::make_shared<RegRefNode>(resultRegIndex),
					std::make_shared<RegRefNode>(resultRegIndex, true) });
		}
	}

	if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
		_insertIns(
			Opcode::STORE,
			{ curMajorContext.curMinorContext.evalDest,
				std::make_shared<RegRefNode>(resultRegIndex, true) });

	curMajorContext.curMinorContext.evaluatedType = resultType;
}
