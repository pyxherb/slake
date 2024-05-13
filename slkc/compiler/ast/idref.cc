#include "../compiler.h"

using namespace slake::slkc;

std::string std::to_string(const slake::slkc::IdRef &ref, slake::slkc::Compiler *compiler) {
	std::string s;
	for (size_t i = 0; i < ref.size(); ++i) {
		if (i)
			s += ".";
		s += ref[i].name;
		if (ref[i].genericArgs.size()) {
			s += "<";
			for (size_t j = 0; j < ref[i].genericArgs.size(); ++j) {
				if (j)
					s += ", ";
				s += std::to_string(ref[i].genericArgs[j], compiler);
			}
			s += ">";
		}
	}
	return s;
}

IdRef slake::slkc::duplicateIdRef(const IdRef &other) {
	IdRef newRef = other;

	for (size_t i = 0; i < other.size(); ++i) {
		for (size_t j = 0; j < newRef[i].genericArgs.size(); ++j) {
			newRef[i].genericArgs[j] = newRef[i].genericArgs[j]->duplicate<TypeNameNode>();
		}
	}

	return newRef;
}
