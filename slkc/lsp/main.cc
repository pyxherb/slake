#include "main.h"
#include <set>

void slake::slkc::lsp::Connection::read() {
	auto self(shared_from_this());
	boost::asio::async_read_until(
		sock,
		buffer,
		"\r\n\r\n",
		[this, self](boost::system::error_code errorCode, size_t nBytesRead) {
			if (!errorCode) {
				std::string bufferString((const char *)buffer.data().data(), nBytesRead);

				try {
					HttpMessage msg = parseHttpMessage(bufferString);

					HttpStatusLine statusLine;
					statusLine.version = "HTTP/1.0";
					statusLine.statusCode = HttpStatus::Ok;
					statusLine.statusText = std::to_string(HttpStatus::Ok);

					reply.startLine = std::to_string(statusLine);

					reply.body = R"(
						<!DOCTYPE html>
						<html>
							<head>
							</head>
							<body>
								<p>OK</p>
							</body>
						</html>
					)";

					reply.header["Content-Length"] = reply.body.size();
					reply.header["Content-Type"] = "text/html";

					write();

					buffer.consume(buffer.size());

					read();
				} catch (HttpParseError e) {
					printf("Http parse error at %d, %d: %s\n", e.line, e.column, e.what());
					puts("Request content:");
					printf("%s\n", bufferString.c_str());

					HttpStatusLine statusLine;
					statusLine.version = "HTTP/1.0";
					statusLine.statusCode = HttpStatus::BadRequest;
					statusLine.statusText = std::to_string(HttpStatus::BadRequest);

					reply.startLine = std::to_string(statusLine);

					reply.body = R"(
						<!DOCTYPE html>
						<html>
							<head>
							</head>
							<body>
								<p>400 Bad request</p>
							</body>
						</html>
					)";

					reply.header["Content-Length"] = reply.body.size();
					reply.header["Content-Type"] = "text/html";

					write();
				}
			} else if (errorCode != boost::asio::error::operation_aborted) {
				stop();
			}
		});
}

void slake::slkc::lsp::Connection::write() {
	auto self(shared_from_this());

	replyString = std::to_string(reply);

	boost::asio::async_write(
		sock,
		boost::asio::buffer(replyString),
		[this, self](boost::system::error_code errorCode, size_t nBytesWritten) {
			if (!errorCode) {
				boost::system::error_code ignoredErrorCode;
				sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignoredErrorCode);
			}

			if (!errorCode != boost::asio::error::operation_aborted) {
				stop();
			}
		});
}

void slake::slkc::lsp::Connection::start() {
	read();
}

void slake::slkc::lsp::Connection::stop() {
	server.connections.erase(shared_from_this());
}

void slake::slkc::lsp::LspServer::accept() {
	acceptor.async_accept(
		sock,
		[this](boost::system::error_code errorCode) {
			if (!acceptor.is_open())
				return;

			if (!errorCode) {
				auto conn = std::make_shared<Connection>(*this, sock);

				connections.insert(conn);

				conn->start();
			}

			accept();
		});
}

int slake::slkc::lsp::LspServer::run() {
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	acceptor.open(endpoint.protocol());
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();

	accept();

	ioContext.run();

	return 0;
}
