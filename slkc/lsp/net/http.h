#ifndef _SLKC_LSP_NET_HTTP_H_
#define _SLKC_LSP_NET_HTTP_H_

#ifdef _MSC_VER
	#pragma push_macro("new")
#endif

#undef new
#include <boost/asio.hpp>

#ifdef _MSC_VER
	#pragma pop_macro("new")
#endif

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

			enum class HttpStatus : uint16_t {
				Ok = 200,
				Created = 201,
				Accepted = 202,
				NoContent = 204,
				MultipleChoices = 300,
				MovedPermanently = 301,
				MovedTemporarily = 302,
				NotModified = 304,
				BadRequest = 400,
				Unauthorized = 401,
				Forbidden = 403,
				NotFound = 404,
				InternalServerError = 500,
				NotImplemented = 501,
				BadGateway = 502,
				ServiceUnavailable = 503
			};

			struct HttpStatusLine {
				std::string version;
				HttpStatus statusCode;
				std::string statusText;
			};

			class HttpParseError : public std::runtime_error {
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

namespace std {
	inline std::string to_string(const slake::slkc::lsp::HttpMessage &msg) {
		std::string s = msg.startLine + "\n";

		for (auto i : msg.header) {
			s += i.first + ": " + i.second + "\n";
		}

		return s + msg.body;
	}
	inline std::string to_string(const slake::slkc::lsp::HttpRequestLine &requestLine) {
		return requestLine.method + " " + requestLine.target + " " + requestLine.version;
	}

	inline std::string to_string(const slake::slkc::lsp::HttpStatusLine &statusLine) {
		return statusLine.version + " " + std::to_string((uint16_t)statusLine.statusCode) + " " + statusLine.statusText;
	}

	inline std::string to_string(slake::slkc::lsp::HttpStatus status) {
		switch (status) {
			case slake::slkc::lsp::HttpStatus::Ok:
				return "OK";
			case slake::slkc::lsp::HttpStatus::Created:
				return "Created";
			case slake::slkc::lsp::HttpStatus::Accepted:
				return "Accepted";
			case slake::slkc::lsp::HttpStatus::NoContent:
				return "No Content";
			case slake::slkc::lsp::HttpStatus::MultipleChoices:
				return "Multiple Choices";
			case slake::slkc::lsp::HttpStatus::MovedPermanently:
				return "Moved Permanently";
			case slake::slkc::lsp::HttpStatus::MovedTemporarily:
				return "Moved Temporarily";
			case slake::slkc::lsp::HttpStatus::NotModified:
				return "Not Modified";
			case slake::slkc::lsp::HttpStatus::BadRequest:
				return "Bad Request";
			case slake::slkc::lsp::HttpStatus::Unauthorized:
				return "Unauthorized";
			case slake::slkc::lsp::HttpStatus::Forbidden:
				return "Forbidden";
			case slake::slkc::lsp::HttpStatus::NotFound:
				return "Not Found";
			case slake::slkc::lsp::HttpStatus::InternalServerError:
				return "Internal Server Error";
			case slake::slkc::lsp::HttpStatus::NotImplemented:
				return "Not Implemented";
			case slake::slkc::lsp::HttpStatus::BadGateway:
				return "Bad Gateway";
			case slake::slkc::lsp::HttpStatus::ServiceUnavailable:
				return "Service Unavailable";
			default:
				throw std::invalid_argument("Invalid HTTP status");
		}
	}
}

#endif
