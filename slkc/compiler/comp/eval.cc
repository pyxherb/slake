#include "../compiler.h"
#include <cmath>

using namespace slake::slkc;

template <typename T>
static shared_ptr<ExprNode> _evalConstUnaryOpExpr(
	UnaryOp op,
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
		case UnaryOp::LNot:
			if constexpr (is_convertible_v<bool, T>) {
				return make_shared<BoolLiteralExprNode>(x->getLocation(), !(x->data));
			} else {
				return {};
			}
		case UnaryOp::Not: {
			if constexpr (is_same_v<T, bool>) {
				return make_shared<LT>(x->getLocation(), !x->data);
			} else if constexpr (is_integral_v<T>) {
				return make_shared<LT>(x->getLocation(), ~x->data);
			} else if constexpr (is_same_v<T, float>) {
				return make_shared<LT>(x->getLocation(), ~(*(uint32_t *)&x->data));
			} else if constexpr (is_same_v<T, double>) {
				return make_shared<LT>(x->getLocation(), ~(*(uint64_t *)&x->data));
			} else
				return {};
		}
		default:
			return {};
	}
}

template <typename T>
static shared_ptr<ExprNode> _evalConstBinaryOpExpr(
	BinaryOp op,
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x,
	shared_ptr<ExprNode> y) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
		case BinaryOp::Add:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data + static_pointer_cast<LT>(y)->data);
			else
				return make_shared<LT>(
					x->getLocation(),
					x->data + static_pointer_cast<LT>(y)->data);
		case BinaryOp::Sub:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data - static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data - static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::Mul:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data * static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data * static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::Div:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(), ((int)x->data) / ((int)static_pointer_cast<LT>(y)->data));
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data / static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::Mod:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					((int)x->data) % ((int)static_pointer_cast<LT>(y)->data));
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data % static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>)
				return make_shared<LT>(
					x->getLocation(),
					fmodf(x->data, static_pointer_cast<LT>(y)->data));
			else if constexpr (is_same_v<T, double>)
				return make_shared<LT>(
					x->getLocation(),
					fmod(x->data, static_pointer_cast<LT>(y)->data));
			else
				return {};
		case BinaryOp::And:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data && static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data & static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) & (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) & (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case BinaryOp::Or:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data || static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data | static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) | (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) | (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case BinaryOp::Xor:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					x->data ^ static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data ^ static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) ^ (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) ^ (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case BinaryOp::LAnd:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data && static_pointer_cast<LT>(y)->data);
			} else
				return {};
		case BinaryOp::LOr:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data || static_pointer_cast<LT>(y)->data);
			} else
				return {};
		case BinaryOp::Lsh:
			if (y->getExprType() != ExprType::I32)
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data << static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data << static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) << (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) << (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case BinaryOp::Rsh:
			if (y->getExprType() != ExprType::I32)
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data >> static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data >> static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) >> (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) >> (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case BinaryOp::Eq:
			if (y->getExprType() != x->getExprType())
				return {};
			return make_shared<BoolLiteralExprNode>(
				x->getLocation(),
				x->data == static_pointer_cast<LT>(y)->data);
		case BinaryOp::Neq:
			if (y->getExprType() != x->getExprType())
				return {};
			return make_shared<BoolLiteralExprNode>(
				x->getLocation(),
				x->data != static_pointer_cast<LT>(y)->data);
		case BinaryOp::Lt:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data < static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::Gt:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data > static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::LtEq:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data <= static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::GtEq:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data >= static_pointer_cast<LT>(y)->data);
			else
				return {};
		case BinaryOp::Subscript:
			if (y->getExprType() != ExprType::I32)
				return {};
			if (static_pointer_cast<I32LiteralExprNode>(y)->data < 0)
				return {};

			if constexpr (is_same_v<T, string>) {
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					x->data[static_pointer_cast<I32LiteralExprNode>(y)->data]);
			} else
				return {};
		default:
			return {};
	}
}

shared_ptr<ExprNode> Compiler::evalConstExpr(shared_ptr<ExprNode> expr) {
	switch (expr->getExprType()) {
		case ExprType::I8:
		case ExprType::I16:
		case ExprType::I32:
		case ExprType::I64:
		case ExprType::U8:
		case ExprType::U16:
		case ExprType::U32:
		case ExprType::U64:
		case ExprType::F32:
		case ExprType::F64:
		case ExprType::String:
		case ExprType::Bool:
			return expr;
		case ExprType::Unary: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);
			if (!evalConstExpr(e->x))
				return {};

			switch (e->x->getExprType()) {
				case ExprType::I32:
					return _evalConstUnaryOpExpr<int32_t>(e->op, static_pointer_cast<I32LiteralExprNode>(e->x));
				case ExprType::I64:
					return _evalConstUnaryOpExpr<int64_t>(e->op, static_pointer_cast<I64LiteralExprNode>(e->x));
				case ExprType::U32:
					return _evalConstUnaryOpExpr<uint32_t>(e->op, static_pointer_cast<U32LiteralExprNode>(e->x));
				case ExprType::U64:
					return _evalConstUnaryOpExpr<uint64_t>(e->op, static_pointer_cast<U64LiteralExprNode>(e->x));
				case ExprType::F32:
					return _evalConstUnaryOpExpr<float>(e->op, static_pointer_cast<F32LiteralExprNode>(e->x));
				case ExprType::F64:
					return _evalConstUnaryOpExpr<double>(e->op, static_pointer_cast<F64LiteralExprNode>(e->x));
				case ExprType::String:
					return _evalConstUnaryOpExpr<string>(e->op, static_pointer_cast<StringLiteralExprNode>(e->x));
				case ExprType::Bool:
					return _evalConstUnaryOpExpr<bool>(e->op, static_pointer_cast<BoolLiteralExprNode>(e->x));
				default:
					return {};
			}
		}
		case ExprType::Binary: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			auto lhs = evalConstExpr(e->lhs), rhs = evalConstExpr(e->rhs);

			if ((!lhs) || (!rhs))
				return {};

			switch (lhs->getExprType()) {
				case ExprType::I32:
					return _evalConstBinaryOpExpr<int32_t>(
						e->op,
						static_pointer_cast<I32LiteralExprNode>(lhs),
						rhs);
				case ExprType::I64:
					return _evalConstBinaryOpExpr<int64_t>(
						e->op,
						static_pointer_cast<I64LiteralExprNode>(lhs),
						rhs);
				case ExprType::U32:
					return _evalConstBinaryOpExpr<uint32_t>(
						e->op,
						static_pointer_cast<U32LiteralExprNode>(lhs),
						rhs);
				case ExprType::U64:
					return _evalConstBinaryOpExpr<uint64_t>(
						e->op,
						static_pointer_cast<U64LiteralExprNode>(lhs),
						rhs);
				case ExprType::F32:
					return _evalConstBinaryOpExpr<float>(
						e->op,
						static_pointer_cast<F32LiteralExprNode>(lhs),
						rhs);
				case ExprType::F64:
					return _evalConstBinaryOpExpr<double>(
						e->op,
						static_pointer_cast<F64LiteralExprNode>(lhs),
						rhs);
				case ExprType::String:
					return _evalConstBinaryOpExpr<string>(
						e->op,
						static_pointer_cast<StringLiteralExprNode>(lhs),
						rhs);
				case ExprType::Bool:
					return _evalConstBinaryOpExpr<bool>(
						e->op,
						static_pointer_cast<BoolLiteralExprNode>(lhs),
						rhs);
				default:
					return {};
			}
		}
		case ExprType::Ternary: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			auto condition = evalConstExpr(e->condition);
			if (condition) {
				if (static_pointer_cast<BoolLiteralExprNode>(castLiteralExpr(condition, Type::Bool))->data)
					return e->x;
				return e->y;
			}

			return {};
		}
		case ExprType::Array: {
			auto e = static_pointer_cast<ArrayExprNode>(expr);
			for (auto i : e->elements)
				if (!evalConstExpr(i))
					return {};
			return e;
		}
		case ExprType::Map: {
			auto e = static_pointer_cast<MapExprNode>(expr);
			for (auto i : e->pairs)
				if ((!evalConstExpr(i.first)) || (!evalConstExpr(i.second)))
					return {};
			return e;
		}
		case ExprType::Ref: {
			auto e = static_pointer_cast<RefExprNode>(expr);

			return {};	// stub
		}
		case ExprType::Cast: {
			auto e = static_pointer_cast<CastExprNode>(expr);
			if (!isLiteralTypeName(e->targetType))
				return {};

			switch (e->getExprType()) {
				case ExprType::I32: {
					switch (e->targetType->getTypeId()) {
						case Type::I8: {
						}
						case Type::I16: {
						}
						case Type::I32: {
						}
						case Type::I64: {
						}
						case Type::U8: {
						}
						case Type::U16: {
						}
						case Type::U32: {
						}
						case Type::U64: {
						}
						case Type::F32: {
						}
						case Type::F64: {
						}
						case Type::Bool: {
						}
					}
				}
				case ExprType::U32: {
				}
				case ExprType::U64: {
				}
				case ExprType::I64: {
				}
				case ExprType::F32: {
				}
				case ExprType::F64: {
				}
				case ExprType::Bool: {
				}
			}
			return evalConstExpr(e->target);
		}
		case ExprType::Match:
			// stub
		default:
			return {};
	}
}

shared_ptr<TypeNameNode> Compiler::evalExprType(shared_ptr<ExprNode> expr) {
	auto t = expr->getExprType();
	switch (t) {
		case ExprType::I8:
			return make_shared<I8TypeNameNode>(Location(), true);
		case ExprType::I16:
			return make_shared<I16TypeNameNode>(Location(), true);
		case ExprType::I32:
			return make_shared<I32TypeNameNode>(Location(), true);
		case ExprType::I64:
			return make_shared<I64TypeNameNode>(Location(), true);
		case ExprType::U8:
			return make_shared<U8TypeNameNode>(Location(), true);
		case ExprType::U16:
			return make_shared<U16TypeNameNode>(Location(), true);
		case ExprType::U32:
			return make_shared<U32TypeNameNode>(Location(), true);
		case ExprType::U64:
			return make_shared<U64TypeNameNode>(Location(), true);
		case ExprType::F32:
			return make_shared<F32TypeNameNode>(Location(), true);
		case ExprType::F64:
			return make_shared<F64TypeNameNode>(Location(), true);
		case ExprType::String:
			return make_shared<StringTypeNameNode>(Location(), true);
		case ExprType::Bool:
			return make_shared<BoolTypeNameNode>(Location(), true);
		case ExprType::Unary: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			uint32_t lhsRegIndex = allocReg();

			auto lhsType = evalExprType(e->x);

			switch (lhsType->getTypeId()) {
				case Type::I8:
				case Type::I16:
				case Type::I32:
				case Type::I64:
				case Type::U8:
				case Type::U16:
				case Type::U32:
				case Type::U64:
				case Type::F32:
				case Type::F64:
					switch (e->op) {
						case UnaryOp::LNot:
						case UnaryOp::Not:
							return make_shared<BoolTypeNameNode>(e->getLocation(), false);
						default:
							return lhsType;
					}

					break;
				case Type::Bool:
					switch (e->op) {
						case UnaryOp::LNot:
						case UnaryOp::Not:
							return make_shared<BoolTypeNameNode>(e->getLocation(), false);
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}

					break;
				case Type::Custom: {
					auto node = resolveCustomTypeName(static_pointer_cast<CustomTypeNameNode>(lhsType).get());

					auto determineOverloading = [this, e](shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> shared_ptr<TypeNameNode> {
						if (auto it = n->scope->members.find(std::to_string(e->op));
							it != n->scope->members.end()) {
							assert(it->second->getNodeType() == NodeType::Fn);
							shared_ptr<FnNode> operatorNode = static_pointer_cast<FnNode>(it->second);
							shared_ptr<FnOverloadingNode> overloading;

							try {
								overloading = argDependentLookup(e->getLocation(), operatorNode.get(), {}, {});
							} catch (FatalCompilationError e) {
								return {};
							}

							return overloading->returnType;
						}
						return {};
					};

					switch (node->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface:
						case NodeType::Trait: {
							uint32_t lhsRegIndex = allocReg(1);

							shared_ptr<MemberNode> n = static_pointer_cast<MemberNode>(node);

							if (!determineOverloading(n, lhsRegIndex))
								throw FatalCompilationError(
									Message(
										e->getLocation(),
										MessageType::Error,
										"No matching operator"));

							break;
						}
						case NodeType::GenericParam: {
							uint32_t lhsRegIndex = allocReg(1);

							shared_ptr<GenericParamNode> n = static_pointer_cast<GenericParamNode>(node);

							shared_ptr<AstNode> curMember;

							curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

							if (curMember->getNodeType() != NodeType::Class)
								throw FatalCompilationError(
									Message(
										n->baseType->getLocation(),
										MessageType::Error,
										"Must be a class"));

							if (auto t = determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex); t)
								return t;

							for (auto i : n->interfaceTypes) {
								curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

								if (curMember->getNodeType() != NodeType::Interface)
									throw FatalCompilationError(
										Message(
											n->baseType->getLocation(),
											MessageType::Error,
											"Must be an interface"));

								if (auto t = determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex); t)
									return t;
							}

							for (auto i : n->traitTypes) {
								curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

								if (curMember->getNodeType() != NodeType::Interface)
									throw FatalCompilationError(
										Message(
											n->baseType->getLocation(),
											MessageType::Error,
											"Must be an interface"));

								if (auto t = determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex); t)
									return t;
							}

							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
							break;
						}
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							e->getLocation(),
							MessageType::Error,
							"No matching operator"));
			}

			break;
		}
		case ExprType::Binary: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			auto lhsType = evalExprType(e->lhs);
			switch (lhsType->getTypeId()) {
				case Type::I8:
				case Type::I16:
				case Type::I32:
				case Type::I64:
				case Type::U8:
				case Type::U16:
				case Type::U32:
				case Type::U64:
				case Type::F32:
				case Type::F64: {
					switch (e->op) {
						case BinaryOp::Add:
						case BinaryOp::Sub:
						case BinaryOp::Mul:
						case BinaryOp::Div:
						case BinaryOp::Mod:
						case BinaryOp::And:
						case BinaryOp::Or:
						case BinaryOp::Xor:
						case BinaryOp::Lsh:
						case BinaryOp::Rsh:
						case BinaryOp::Assign:
						case BinaryOp::AssignAdd:
						case BinaryOp::AssignSub:
						case BinaryOp::AssignMul:
						case BinaryOp::AssignDiv:
						case BinaryOp::AssignMod:
						case BinaryOp::AssignAnd:
						case BinaryOp::AssignOr:
						case BinaryOp::AssignXor:
						case BinaryOp::AssignLsh:
						case BinaryOp::AssignRsh:
							return lhsType;
						case BinaryOp::LAnd:
						case BinaryOp::LOr:
						case BinaryOp::Eq:
						case BinaryOp::Neq:
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq:
						case BinaryOp::Lt:
						case BinaryOp::Gt:
						case BinaryOp::LtEq:
						case BinaryOp::GtEq:
							return make_shared<BoolTypeNameNode>(Location(), true);
						case BinaryOp::Swap:
						default:
							assert(false);
					}

					break;
				}
				case Type::Bool: {
					switch (e->op) {
						case BinaryOp::LAnd:
						case BinaryOp::LOr:
						case BinaryOp::Eq:
						case BinaryOp::Neq:
							return make_shared<BoolTypeNameNode>(Location(), true);
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
				}
				case Type::String: {
					switch (e->op) {
						case BinaryOp::Add:
							return make_shared<StringTypeNameNode>(Location(), true);
						case BinaryOp::Subscript:
							return make_shared<U8TypeNameNode>(Location(), true);
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
					break;
				}
				case Type::WString: {
					switch (e->op) {
						case BinaryOp::Add:
							return make_shared<WStringTypeNameNode>(Location(), true);
						case BinaryOp::Subscript:
							return make_shared<U32TypeNameNode>(Location(), true);
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
					break;
				}
				case Type::Custom: {
					auto node = resolveCustomTypeName(static_pointer_cast<CustomTypeNameNode>(lhsType).get());
					auto rhsType = evalExprType(e->lhs);

					auto determineOverloading = [this, e, rhsType](shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> shared_ptr<TypeNameNode> {
						if (auto it = n->scope->members.find(std::to_string(e->op));
							it != n->scope->members.end()) {
							assert(it->second->getNodeType() == NodeType::Fn);
							shared_ptr<FnNode> operatorNode = static_pointer_cast<FnNode>(n);
							auto overloading = operatorNode->overloadingRegistries[0];

							Ref fullName;
							_getFullName(operatorNode.get(), fullName);

							try {
								auto overloading = argDependentLookup(e->getLocation(), operatorNode.get(), { rhsType }, {});

								return overloading->returnType;
							} catch (FatalCompilationError e) {
								return {};
							}
						}
						return {};
					};

					switch (node->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface:
						case NodeType::Trait: {
							uint32_t lhsRegIndex = allocReg(1);

							switch (e->op) {
								case BinaryOp::Assign: {
									uint32_t rhsRegIndex = allocReg(1);

									compileExpr(
										e->lhs,
										EvalPurpose::LValue,
										make_shared<RegRefNode>(lhsRegIndex));

									compileExpr(
										e->rhs,
										EvalPurpose::RValue,
										make_shared<RegRefNode>(rhsRegIndex));

									if (!isSameType(lhsType, rhsType)) {
										if (!isTypeNamesConvertible(rhsType, lhsType))
											throw FatalCompilationError(
												{ e->rhs->getLocation(),
													MessageType::Error,
													"Incompatible operand types" });

										compileExpr(
											make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
											EvalPurpose::LValue,
											make_shared<RegRefNode>(rhsRegIndex));
									}

									curFn->insertIns(
										Opcode::STORE,
										make_shared<RegRefNode>(lhsRegIndex, true),
										make_shared<RegRefNode>(rhsRegIndex, true));

									break;
								}
								default: {
									shared_ptr<MemberNode> n = static_pointer_cast<MemberNode>(node);

									if (!determineOverloading(n, lhsRegIndex))
										throw FatalCompilationError(
											Message(
												e->getLocation(),
												MessageType::Error,
												"No matching operator"));
								}
							}

							break;
						}
						case NodeType::GenericParam: {
							uint32_t lhsRegIndex = allocReg(1);

							shared_ptr<GenericParamNode> n = static_pointer_cast<GenericParamNode>(node);

							switch (e->op) {
								case BinaryOp::Assign: {
									uint32_t rhsRegIndex = allocReg(1);

									compileExpr(
										e->lhs,
										EvalPurpose::LValue,
										make_shared<RegRefNode>(lhsRegIndex));

									compileExpr(
										e->rhs,
										EvalPurpose::RValue,
										make_shared<RegRefNode>(rhsRegIndex));

									if (!isSameType(lhsType, rhsType)) {
										if (!isTypeNamesConvertible(rhsType, lhsType))
											throw FatalCompilationError(
												{ e->rhs->getLocation(),
													MessageType::Error,
													"Incompatible operand types" });

										compileExpr(
											make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
											EvalPurpose::LValue,
											make_shared<RegRefNode>(rhsRegIndex));
									}

									curFn->insertIns(
										Opcode::STORE,
										make_shared<RegRefNode>(lhsRegIndex, true),
										make_shared<RegRefNode>(rhsRegIndex, true));

									break;
								}
								default: {
									shared_ptr<AstNode> curMember;

									curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

									if (curMember->getNodeType() != NodeType::Class)
										throw FatalCompilationError(
											Message(
												n->baseType->getLocation(),
												MessageType::Error,
												"Must be a class"));

									auto savedCurFn = curFn->duplicate<CompiledFnNode>();
									auto savedCurMajorContext = curMajorContext;

									if (auto t = determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex); t)
										return t;

									for (auto i : n->interfaceTypes) {
										curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

										if (curMember->getNodeType() != NodeType::Interface)
											throw FatalCompilationError(
												Message(
													n->baseType->getLocation(),
													MessageType::Error,
													"Must be an interface"));

										if (auto t = determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex); t)
											return t;
									}

									for (auto i : n->traitTypes) {
										curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

										if (curMember->getNodeType() != NodeType::Interface)
											throw FatalCompilationError(
												Message(
													n->baseType->getLocation(),
													MessageType::Error,
													"Must be an interface"));

										if (auto t = determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex); t)
											return t;
									}

									throw FatalCompilationError(
										Message(
											e->getLocation(),
											MessageType::Error,
											"No matching operator"));
								}
							}
							break;
						}
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							e->getLocation(),
							MessageType::Error,
							"No matching operator"));
			}

			break;
		}
		case ExprType::Ref: {
			auto e = static_pointer_cast<RefExprNode>(expr);
			deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;

			if (resolveRef(e->ref, resolvedParts)) {
				switch (resolvedParts.back().second->getNodeType()) {
					case NodeType::Var:
						return static_pointer_cast<VarNode>(resolvedParts.back().second)->type;
					case NodeType::LocalVar:
						return static_pointer_cast<LocalVarNode>(resolvedParts.back().second)->type;
					case NodeType::ArgRef:
						return curFn->params.at(static_pointer_cast<ArgRefNode>(resolvedParts.back().second)->index)->type;
					case NodeType::Fn: {
						shared_ptr<FnNode> fn = static_pointer_cast<FnNode>(resolvedParts.back().second);

						if (fn->overloadingRegistries.size() == 1) {
							shared_ptr<FnTypeNameNode> type;

							deque<shared_ptr<TypeNameNode>> paramTypes;
							for (auto i : fn->overloadingRegistries[0]->params) {
								paramTypes.push_back(i->type);
							}

							type = make_shared<FnTypeNameNode>(fn->overloadingRegistries[0]->returnType, paramTypes);
							return type;
						}

						if (curMajorContext.curMinorContext.isArgTypesSet) {
							auto overloading = argDependentLookup(e->ref[0].loc, fn.get(), curMajorContext.curMinorContext.argTypes, resolvedParts.back().first.back().genericArgs);

							deque<shared_ptr<TypeNameNode>> paramTypes;
							for (auto i : overloading->params)
								paramTypes.push_back(i->type);

							return make_shared<FnTypeNameNode>(overloading->returnType, paramTypes);
						}

						throw FatalCompilationError(
							Message(
								e->getLocation(),
								MessageType::Error,
								"No matching function was found"));
					}
					case NodeType::Class:
					case NodeType::Interface:
					case NodeType::Trait:
						throw FatalCompilationError(
							{ e->getLocation(),
								MessageType::Error,
								"`" + to_string(e->ref, this) + "' is a type" });
					case NodeType::ThisRef: {
						auto owner = curMajorContext.curMinorContext.curScope->owner;

						switch (owner->getNodeType()) {
							case NodeType::Class:
							case NodeType::Interface:
							case NodeType::Trait:
								return curMajorContext.thisType;
							default:
								throw FatalCompilationError(
									Message(
										e->getLocation(),
										MessageType::Error,
										"Cannot use this reference in this context"));
						}
					}
					default:
						assert(false);
				}
			} else {
				throw FatalCompilationError(
					{ e->getLocation(),
						MessageType::Error,
						"Identifier not found: `" + to_string(e->ref, this) + "'" });
			}

			break;
		}
		case ExprType::New:
			return static_pointer_cast<NewExprNode>(expr)->type;
		case ExprType::Call: {
			auto e = static_pointer_cast<CallExprNode>(expr);

			deque<shared_ptr<TypeNameNode>> argTypes;

			for (auto &i : e->args) {
				argTypes.push_back(evalExprType(i));
			}

			pushMinorContext();
			curMajorContext.curMinorContext.isArgTypesSet = true;
			curMajorContext.curMinorContext.argTypes = argTypes;

			auto t = evalExprType(e->target);

			popMinorContext();

			switch (t->getTypeId()) {
				case Type::Fn:
					return static_pointer_cast<FnTypeNameNode>(t)->returnType;
				case Type::Custom:
					// stub
				default:
					throw FatalCompilationError(
						{ e->getLocation(),
							MessageType::Error,
							"Expression is not callable" });
			}
		}
		case ExprType::Cast: {
			auto e = static_pointer_cast<CastExprNode>(expr);

			return e->targetType;
		}
	}
	assert(false);
}
