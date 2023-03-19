#include "../rt.h"

Slake::Fn::Fn(FnFlags flags, std::size_t nIns) : _flags(_flags) {
	if (!nIns)
		throw std::invalid_argument("Invalid instruction count");
	_nIns = nIns;
	_body = new Instruction[nIns]{};
}

Slake::Fn::~Fn() {
	delete[] _body;
}

std::uint32_t Slake::Fn::getInsCount() const {
	return _nIns;
}

const Slake::Instruction* Slake::Fn::getBody() const {
	return _body;
}

Slake::Instruction* Slake::Fn::getBody() {
	return _body;
}

Slake::FnFlags Slake::Fn::getFlags() const {
	return _flags;
}
