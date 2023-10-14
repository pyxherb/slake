#include "expr.h"

using namespace slake::slkc;

std::string std::to_string(UnaryOp op) {
	switch (op) {
		case OP_NOT:
			return "!";
		case OP_REV:
			return "~";
		case OP_INCF:
		case OP_INCB:
			return "--";
		case OP_DECF:
		case OP_DECB:
			return "--";
		default:
			throw logic_error("Unrecognized operator type");
	}
}

std::string std::to_string(BinaryOp op) {
	switch (op) {
		case OP_ADD:
			return "+";
		case OP_SUB:
			return "-";
		case OP_MUL:
			return "*";
		case OP_DIV:
			return "/";
		case OP_MOD:
			return "%";
		case OP_AND:
			return "&";
		case OP_OR:
			return "|";
		case OP_XOR:
			return "^";
		case OP_LAND:
			return "&&";
		case OP_LOR:
			return "||";
		case OP_LSH:
			return "<<";
		case OP_RSH:
			return ">>";
		case OP_SWAP:
			return "<=>";
		case OP_ASSIGN:
			return "=";
		case OP_ASSIGN_ADD:
			return "+=";
		case OP_ASSIGN_SUB:
			return "-=";
		case OP_ASSIGN_MUL:
			return "*=";
		case OP_ASSIGN_DIV:
			return "/=";
		case OP_ASSIGN_MOD:
			return "%=";
		case OP_ASSIGN_AND:
			return "&=";
		case OP_ASSIGN_OR:
			return "|=";
		case OP_ASSIGN_XOR:
			return "^=";
		case OP_ASSIGN_LAND:
			return "&&=";
		case OP_ASSIGN_LOR:
			return "||=";
		case OP_ASSIGN_LSH:
			return "<<=";
		case OP_ASSIGN_RSH:
			return ">>=";
		case OP_EQ:
			return "==";
		case OP_NEQ:
			return "!=";
		case OP_STRICTEQ:
			return "===";
		case OP_STRICTNEQ:
			return "!==";
		case OP_LT:
			return "<";
		case OP_GT:
			return ">";
		case OP_LTEQ:
			return "<=";
		case OP_GTEQ:
			return ">=";
		case OP_SUBSCRIPT:
			return "[]";
		default:
			throw logic_error("Unrecognized operator type");
	}
}