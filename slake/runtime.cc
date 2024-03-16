#include "runtime.h"

using namespace slake;

MinorFrame::MinorFrame(uint32_t nLocalVars, uint32_t nRegs) : nLocalVars(nLocalVars), nRegs(nRegs) {
}

MajorFrame::MajorFrame(Runtime *rt) {
	minorFrames.push_back(MinorFrame(0, 0));
}

Runtime::Runtime(RuntimeFlags flags) : _flags(flags) {
	_rootValue = new RootValue(this);
}

Runtime::~Runtime() {
	_rootValue = nullptr;

	gc();

	assert(!_createdValues.size());
	assert(!_szMemInUse);
}

std::string Runtime::mangleName(
	std::string name,
	std::deque<Type> params,
	GenericArgList genericArgs,
	bool isConst) const {
	std::string s = name;

	if (isConst)
		s += "$const";

	for (auto i : params)
		s += "$" + std::to_string(i, this);

	for (auto i : genericArgs)
		s += "?" + std::to_string(i, this);

	return s;
}

std::string Runtime::getFullName(const MemberValue *v) const {
	std::string s;
	do {
		switch (v->getType().typeId) {
			case TypeId::Object:
				v = (const MemberValue *)((ObjectValue *)v)->getType().getCustomTypeExData();
				break;
		}
		s = v->getName() + (s.empty() ? "" : "." + s);
	} while ((Value *)(v = (const MemberValue *)v->getParent()) != _rootValue);
	return s;
}

std::string Runtime::getFullName(const RefValue *v) const {
	return std::to_string(v);
}

std::deque<RefEntry> Runtime::getFullRef(const MemberValue* v) const {
	std::deque<RefEntry> entries;
	do {
		switch (v->getType().typeId) {
			case TypeId::Object:
				v = (const MemberValue *)((ObjectValue *)v)->getType().getCustomTypeExData();
				break;
		}
		entries.push_back({ v->getName(), v->_genericArgs });
	} while ((Value *)(v = (const MemberValue *)v->getParent()) != _rootValue);
	return entries;
}
