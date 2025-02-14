#include "../parser.h"

using namespace slake;
using namespace slake::slkc;

void Parser::resetLineCommentDocumentation() {
	curDocStringLines.clear();
	curMinDocIndentLevel = SIZE_MAX;
	isLastTokenNewline = false;
}

void Parser::updateLineCommentDocumentation(Token *token) {
	std::string content = token->text.substr(3);
	size_t i = 0, indentLevel = 0;
	while (i < content.size()) {
		switch (content[i]) {
		case ' ':
			indentLevel += 1;
			break;
		case '\t':
			// TODO: Replace 4 with the indent size specified by the user
			indentLevel += 4;
			break;
		default:
			goto indentLevelDetectionEnd;
		}
		++i;
	}
indentLevelDetectionEnd:

	// We have to ignore lines containing nothing or whitespaces only.
	if (i &&
		(i != content.size())) {
		if (indentLevel < curMinDocIndentLevel) {
			const size_t indentDiff = curMinDocIndentLevel - indentLevel;
			for (auto &j : curDocStringLines)
				j = std::string(' ', indentDiff) + j;
			curMinDocIndentLevel = indentLevel;
		}
	}

	curDocStringLines.push_back(content.substr(i));
}

std::string Parser::extractLineCommentDocumentation() {
	std::string result;

	for (auto &i : curDocStringLines) {
		result += std::move(i);
	}

	resetLineCommentDocumentation();

	return result;
}
