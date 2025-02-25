#include "expr.h"

using namespace slake::slkc;

std::string std::to_string(UnaryOp op) {
	switch (op) {
		case UnaryOp::LNot:
			return "!";
		case UnaryOp::Not:
			return "~";
		case UnaryOp::Neg:
			return "-";
		default:
			throw logic_error("Unrecognized operator type");
	}
}

std::string std::to_string(BinaryOp op) {
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
		case BinaryOp::Lsh:
			return "<<";
		case BinaryOp::Rsh:
			return ">>";
		case BinaryOp::Assign:
			return "=";
		case BinaryOp::AssignAdd:
			return "+=";
		case BinaryOp::AssignSub:
			return "-=";
		case BinaryOp::AssignMul:
			return "*=";
		case BinaryOp::AssignDiv:
			return "/=";
		case BinaryOp::AssignMod:
			return "%=";
		case BinaryOp::AssignAnd:
			return "&=";
		case BinaryOp::AssignOr:
			return "|=";
		case BinaryOp::AssignXor:
			return "^=";
		case BinaryOp::AssignLsh:
			return "<<=";
		case BinaryOp::AssignRsh:
			return ">>=";
		case BinaryOp::Eq:
			return "==";
		case BinaryOp::Neq:
			return "!=";
		case BinaryOp::StrictEq:
			return "===";
		case BinaryOp::StrictNeq:
			return "!==";
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
			throw logic_error("Unrecognized operator type");
	}
}
