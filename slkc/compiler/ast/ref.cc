#include "../compiler.h"

using namespace slake::slkc;

string std::to_string(const slake::slkc::Ref &ref, slake::slkc::Compiler *compiler) {
	string s;
	for (size_t i = 0; i < ref.size(); ++i) {
		if (i)
			s += ".";
		s += ref[i].name;
		if (ref[i].genericArgs.size()) {
			s += "<";
			for (size_t j = 0; j < ref[i].genericArgs.size(); ++j) {
				if (j)
					s += ", ";
				s += to_string(ref[i].genericArgs[j], compiler);
			}
			s += ">";
		}
	}
	return s;
}

string std::to_string(const slake::slkc::ModuleRef &ref) {
	string s;
	for (size_t i = 0; i < ref.size(); ++i) {
		if (i)
			s += ".";
		s += ref[i].name;
	}
	return s;
}

bool operator<(slake::slkc::ModuleRef lhs, slake::slkc::ModuleRef rhs) {
	if (lhs.size() < rhs.size())
		return true;
	if (lhs.size() > rhs.size())
		return false;

	for (size_t i = 0; i < lhs.size(); ++i) {
		if (!(lhs[i].name < rhs[i].name))
			return false;
	}

	return true;
}

slake::slkc::Ref slake::slkc::toRegularRef(const ModuleRef& ref) {
	Ref result;

	for(auto i : ref) {
		result.push_back({i.loc,i.name});
	}

	return result;
}

ModuleRef slake::slkc::toModuleRef(const Ref &ref) {
	ModuleRef result;

	for(auto i : ref) {
		result.push_back({i.loc, i.name});
	}

	return result;
}
