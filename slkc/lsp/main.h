#ifndef _SLKC_LSP_MAIN_H_
#define _SLKC_LSP_MAIN_H_

#include "net/http.h"
#include <boost/asio.hpp>
#include <memory>
#include <set>

namespace slake {
	namespace slkc {
		namespace lsp {
			using Handler = std::function<HttpMessage(HttpMessage msg)>;

			class Connection;

			class LspServer {
			public:
				uint16_t port;
				boost::asio::io_context ioContext;
				boost::asio::ip::tcp::acceptor acceptor;
				boost::asio::ip::tcp::socket sock;
				std::set<std::shared_ptr<Connection>> connections;

				inline LspServer(uint16_t port)
					: port(port),
					  acceptor(ioContext),
					  sock(ioContext) {
				}

				void accept();
				int run();
			};

			class Connection : public std::enable_shared_from_this<Connection> {
			public:
				boost::asio::ip::tcp::socket sock;
				boost::asio::streambuf buffer;
				LspServer &server;
				HttpMessage reply;
				std::string replyString;

				size_t contentLength = 0, szContentsRead = 0;
				bool isContentLongEnough = false;

				inline Connection(LspServer &server, boost::asio::ip::tcp::socket &sock)
					: server(server), sock(std::move(sock)) {}
				void read();
				void write();
				void start();
				void stop();
			};
		}
	}
}

#endif
