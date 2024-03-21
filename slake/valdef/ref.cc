#include <slake/runtime.h>

using namespace slake;

slake::RefValue::RefValue(Runtime *rt)
	: Value(rt) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
}

RefValue::~RefValue() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
}

Value *RefValue::duplicate() const {
	RefValue *v = new RefValue(_rt);
	*v = *this;

	return (Value *)v;
}

std::string std::to_string(const slake::RefValue *ref) {
	string s;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto scope = ref->entries[i];

		if (i)
			s += ".";
		s += scope.name;

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += to_string(scope.genericArgs[j], ref->getRuntime());
			}
			s += ">";
		}
	}
	return s;
}
