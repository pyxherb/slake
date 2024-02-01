#include "main.h"

#include <boost/asio.hpp>

using namespace boost;
using namespace boost::asio;
using ip::tcp;

int slake::slkc::lsp::lspServerMain(uint16_t port) {
	io_context io;
	tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));

	tcp::socket sock(io);
	acceptor.accept(sock);

	try {

	}
	catch (std::exception e) {
		printf("Exception:\n%s\n", e.what());
	}

	sock.close();

	return 0;
}
