#ifndef _SLKC_LSP_NET_HTTP_H_
#define _SLKC_LSP_NET_HTTP_H_

#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>

namespace slake {
	namespace slkc {
		namespace lsp {
			struct HttpMessage {
				std::string startLine;
				std::unordered_map<std::string, std::string> header;
				std::string body;
			};

			struct HttpRequestLine {
				std::string method;
				std::string target;
				std::string version;
			};

			struct HttpStatusLine {
				std::string version;
				uint16_t statusCode;
				std::string statusText;
			};

			class HttpParseError : std::runtime_error {
			public:
				size_t line, column;

				inline HttpParseError(const std::string &msg, size_t line, size_t column) : runtime_error(msg), line(line), column(column) {}
				virtual ~HttpParseError() = default;
			};

			HttpMessage parseHttpMessage(std::string_view s);
			HttpRequestLine parseHttpRequestLine(std::string_view s);
			HttpStatusLine parseHttpStatusLine(std::string_view s);
		}
	}
}

#endif
