#include <slake/runtime.h>

using namespace slake;

slake::IdRefObject::IdRefObject(Runtime *rt)
	: Object(rt) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
}

IdRefObject::~IdRefObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
}

Object *IdRefObject::duplicate() const {
	IdRefObject *v = new IdRefObject(_rt);
	*v = *this;

	return (Object *)v;
}

std::string std::to_string(const slake::IdRefObject *ref) {
	string s;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &scope = ref->entries[i];

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
