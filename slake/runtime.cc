#include "runtime.h"

using namespace Slake;

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
		if (d && i->getType() == ValueType::OBJECT)
			d->call(0, nullptr);
	}
	destructingThreads.erase(std::this_thread::get_id());

	while (_createdValues.size())
		delete *_createdValues.begin();
}
