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
