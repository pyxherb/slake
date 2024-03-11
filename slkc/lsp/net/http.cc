#include "http.h"

using namespace slake::slkc::lsp;

enum class HttpMessageParseState : uint8_t {
	Initial = 0,

	HeaderValueWhitespaces,
	HeaderValue
};

HttpMessage slake::slkc::lsp::parseHttpMessage(std::string_view s) {
	HttpMessage msg{};
	HttpMessageParseState state = HttpMessageParseState::Initial;

	size_t i = 0, line = 0, column = 0;
	char c;

	while (i < s.size()) {
		c = s[i++];

		++column;

		msg.startLine += c;

		if (c == '\n')
			goto startLineParseEnd;
	}

	throw HttpParseError("Prematured end of message", line, column);

startLineParseEnd:
	++line, column = 0;

	std::pair<std::string, std::string> curHeader;

	while (i < s.size()) {
		switch (state) {
			case HttpMessageParseState::Initial: {
				c = s[i++];
				++column;

				switch (c) {
					case ':':
						state = HttpMessageParseState::HeaderValueWhitespaces;
						break;
					case '\r':
					case '\n':
						if (curHeader.first.empty())
							goto headersParseEnd;
						throw HttpParseError("Prematured end of line", line, column);
					default:
						curHeader.first += c;
				}
				break;
			}
			case HttpMessageParseState::HeaderValueWhitespaces: {
				c = s[i];
				switch (c) {
					case ' ':
					case '\t':
						++i;
						++column;
						break;
					case '\r':
					case '\n':
						throw HttpParseError("Prematured end of line", line, column);
					default:
						state = HttpMessageParseState::HeaderValue;
				}

				break;
			}
			case HttpMessageParseState::HeaderValue: {
				c = s[i++];

				switch (c) {
					case '\n':
						msg.header.insert(curHeader);
						curHeader = {};

						state = HttpMessageParseState::Initial;
						++line, column = 0;
						break;
					default:
						curHeader.second += c;
				}
			}
		}
	}

	throw HttpParseError("Prematured end of message", line, column);

headersParseEnd:;
	msg.body = s.substr(i);

	return msg;
}

HttpRequestLine slake::slkc::lsp::parseHttpRequestLine(std::string_view s) {
	HttpRequestLine requestLine{};
	size_t i = 0;
	char c;

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto methodParseEnd;
			case '\n':
				throw HttpParseError("Unexpected end of line", 0, i - 1);
			default:
				requestLine.method += c;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
methodParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto methodWhitespaceParseEnd;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
methodWhitespaceParseEnd:

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto targetParseEnd;
			case '\n':
				throw HttpParseError("Unexpected end of line", 0, i - 1);
			default:
				requestLine.target += c;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
targetParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto targetWhitespaceParseEnd;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
targetWhitespaceParseEnd:

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case '\n':
				goto versionParseEnd;
			default:
				requestLine.version += c;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
versionParseEnd:

	return requestLine;
}

HttpStatusLine slake::slkc::lsp::parseHttpStatusLine(std::string_view s) {
	HttpStatusLine statusLine{};
	size_t i = 0;
	char c;

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto versionParseEnd;
			case '\n':
				throw HttpParseError("Unexpected end of line", 0, i - 1);
			default:
				statusLine.version += c;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
versionParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto versionWhitespaceParseEnd;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
versionWhitespaceParseEnd:

	while (i < s.size()) {
		c = s[i++];

		switch (c) {
			case ' ':
			case '\t':
				goto statusCodeParseEnd;
			case '\n':
				throw HttpParseError("Unexpected end of line", 0, i - 1);
			default:
				((uint16_t &)statusLine.statusCode) += c;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
statusCodeParseEnd:

	while (i < s.size()) {
		switch (s[i]) {
			case ' ':
			case '\t':
				++i;
				break;
			default:
				goto statusCodeWhitespaceParseEnd;
		}
	}
	throw HttpParseError("Prematured end of message", 0, i - 1);
statusCodeWhitespaceParseEnd:

	statusLine.statusText = s.substr(i);
	return statusLine;
}
