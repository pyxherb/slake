#include "../rt.h"
#include "runtime.h"
#include "rt.h"

Slake::Fn::Fn(FnFlags flags, std::size_t nIns) : _flags(_flags), _nIns(nIns) {
	if (!nIns)
		throw std::invalid_argument("Invalid function length");
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

std::function<void()>& Slake::Fn::getNativeBody() const {
	// TODO: 在此处插入 return 语句
}

Slake::FnFlags Slake::Fn::getFlags() const {
	return _flags;
}
