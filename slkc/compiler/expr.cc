#include "expr.hh"

using namespace Slake::Compiler;

std::string std::to_string(UnaryOp op) {
	switch (op) {
		case UnaryOp::INC_F:
		case UnaryOp::INC_B:
			return "++";
		case UnaryOp::DEC_F:
		case UnaryOp::DEC_B:
			return "--";
		case UnaryOp::NEG:
			return "-";
		case UnaryOp::NOT:
			return "!";
		case UnaryOp::REV:
			return "~";
	}
	throw std::invalid_argument("Invalid unary operator");
}

std::string std::to_string(BinaryOp op) {
	switch (op) {
		case BinaryOp::ADD:
			return "+";
		case BinaryOp::SUB:
			return "-";
		case BinaryOp::MUL:
			return "*";
		case BinaryOp::DIV:
			return "/";
		case BinaryOp::MOD:
			return "%";
		case BinaryOp::AND:
			return "&";
		case BinaryOp::OR:
			return "|";
		case BinaryOp::XOR:
			return "^";
		case BinaryOp::LAND:
			return "&&";
		case BinaryOp::LOR:
			return "||";
		case BinaryOp::EQ:
			return "==";
		case BinaryOp::NEQ:
			return "!=";
		case BinaryOp::LSH:
			return "<<";
		case BinaryOp::RSH:
			return ">>";
		case BinaryOp::GTEQ:
			return ">=";
		case BinaryOp::LTEQ:
			return "<=";
		case BinaryOp::GT:
			return ">";
		case BinaryOp::LT:
			return "<";
		case BinaryOp::ASSIGN:
			return "=";
		case BinaryOp::ADD_ASSIGN:
			return "+=";
		case BinaryOp::SUB_ASSIGN:
			return "-=";
		case BinaryOp::MUL_ASSIGN:
			return "*=";
		case BinaryOp::DIV_ASSIGN:
			return "/=";
		case BinaryOp::MOD_ASSIGN:
			return "%=";
		case BinaryOp::AND_ASSIGN:
			return "&=";
		case BinaryOp::OR_ASSIGN:
			return "|=";
		case BinaryOp::XOR_ASSIGN:
			return "^=";
		case BinaryOp::LSH_ASSIGN:
			return "<<=";
		case BinaryOp::RSH_ASSIGN:
			return ">>=";
		default:
			throw invalid_argument("Invalid binary operator");
	}
}

std::string std::to_string(Slake::Compiler::MatchCase c) {
	return std::to_string(*c.first) + " : " + std::to_string(*c.second);
}
