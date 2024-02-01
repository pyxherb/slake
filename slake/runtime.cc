#include "runtime.h"

using namespace slake;

MinorFrame::MinorFrame(Runtime *rt) {
}

MajorFrame::MajorFrame(Runtime *rt) {
	minorFrames.push_back(MinorFrame(rt));
}

Runtime::Runtime(RuntimeFlags flags) : _flags(flags) {
	_rootValue = new RootValue(this);
	_rootValue->incRefCount();
}

Runtime::~Runtime() {
	gc();

	// Execute destructors for all destructible objects.
	destructingThreads.insert(std::this_thread::get_id());
	for (auto i : _createdValues) {
		auto d = i->getMember("delete");
		if (d && i->getType() == TypeId::OBJECT)
			d->call({});
	}
	destructingThreads.erase(std::this_thread::get_id());

	delete _rootValue;

	_flags |= _RT_DELETING;

	while (_createdValues.size())
		delete *(_createdValues.begin());

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
			case TypeId::OBJECT:
				v = (const MemberValue *)((ObjectValue *)v)->getType().getCustomTypeExData().get();
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
			case TypeId::OBJECT:
				v = (const MemberValue *)((ObjectValue *)v)->getType().getCustomTypeExData().get();
				break;
		}
		entries.push_back({ v->getName(), v->_genericArgs });
	} while ((Value *)(v = (const MemberValue *)v->getParent()) != _rootValue);
	return entries;
}
