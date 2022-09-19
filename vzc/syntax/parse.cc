#include "parse.h"

std::shared_ptr<Vzc::Syntax::VzcParser::Expr> Vzc::Syntax::VzcParser::parseExpr(SymList::iterator& i) {
	std::shared_ptr<Expr> expr;
	auto curSym = _nextSym(i);
	switch(curSym->token) {
		case TOK_ID: {
			auto curSym = _nextSym(i);
			switch(curSym->token) {

			}
			break;
		}
	}

	return expr;
}

void Vzc::Syntax::VzcParser::parseAttrib(SymList::iterator& i) {
	auto curSym = _nextSym(i, TOK_ID);

	std::string attribName =
					std::static_pointer_cast<IdSym>(curSym)->data;
	_nextSym(i, '(');
}

void Vzc::Syntax::VzcParser::parseImport(SymList::iterator& i) {
	auto curSym = _nextSym(i, TOK_ID);

	std::string importName =
					std::static_pointer_cast<IdSym>(curSym)->data,
				importSrc;
	bool searchIncludeDir = false;
	_nextSym(i, '=', "expecting '='");
	if ((curSym = _nextSym(i))->token == '@') {
		searchIncludeDir = true;
		importSrc = std::static_pointer_cast<StringLiteralSym>(_nextSym(i, TOK_STRING_L, "expecting literal string"))->data;
	} else {
		importSrc = std::static_pointer_cast<StringLiteralSym>(curSym)->data;
	}

	printf("import %s = %s\"%s\"\n", importName.c_str(), searchIncludeDir ? "@" : "", importSrc.c_str());
	if ((_nextSym(i, { ',', '}' }, "expecting ',' or '}'"))->token == ',')
		parseImport(i);
}

void Vzc::Syntax::VzcParser::parse(SymList& syms) {
	for (auto i = syms.begin(); i != syms.end();) {
		auto curSym = _nextSym(i);

		switch (curSym->token) {
			case KW_IMPORT:
				_nextSym(i, '{');
				parseImport(i);
				break;
			case KW_PUB:
				curSym = _nextSym(i, { KW_FN });
				break;
			default:
				throw SyntaxError(curSym);
		}
	}
}
