#include "../compiler.h"

using namespace slake::slkc;

string std::to_string(const slake::slkc::Ref &ref) {
	string s;
	for (const auto &i : ref) {
		s += i.name;
		if (i.genericArgs.size()) {
			s += "<";
			for (size_t j = 0; j < i.genericArgs.size(); ++j) {
				if (j)
					s += ", ";
				s += to_string(i.genericArgs[j]);
			}
			s += ">";
		}
	}
    return s;
}