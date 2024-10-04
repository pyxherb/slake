#include "../compiler.h"

using namespace slake::slkc;

std::string std::to_string(const slake::slkc::IdRefEntries &ref, slake::slkc::Compiler *compiler, bool forMangling) {
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
				s += std::to_string(ref[i].genericArgs[j], compiler, forMangling);
			}
			s += ">";
		}
	}
	return s;
}

IdRefNode::IdRefNode(const IdRefEntries &entries) : entries(entries) {}
IdRefNode::IdRefNode(IdRefEntries &&entries) : entries(entries) {}

std::shared_ptr<AstNode> IdRefNode::doDuplicate() {
	IdRefEntries newRef = entries;

	for (size_t i = 0; i < entries.size(); ++i) {
		for (size_t j = 0; j < newRef[i].genericArgs.size(); ++j) {
			newRef[i].genericArgs[j] = newRef[i].genericArgs[j]->duplicate<TypeNameNode>();
		}
	}

	return std::make_shared<IdRefNode>(std::move(newRef));
}
