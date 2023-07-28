#include <slake/runtime.h>

using namespace slake;

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

		if (scope.genericArgs.size()) {
			s += "<";
			for (size_t j = 0; j < scope.genericArgs.size(); ++j) {
				if (j)
					s += ",";
				s += to_string(scope.genericArgs[j], ref->getRuntime());
			}
			s += ">";
		}
	}
	return s;
}
