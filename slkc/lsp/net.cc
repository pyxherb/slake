#include "net.h"

#ifdef _WIN32
	#include <WinSock2.h>
#else
	#include <sys/socket.h>
#endif

void Slake::Server::init() {
#ifdef _WIN32
	{
		WSADATA wsaData;
		if (!WSAStartup(MAKEWORD(2, 2), &wsaData))
			throw std::runtime_error("Error initializing WSA");
	}
#else
	throw std::logic_error("LSP server was not implemented yet");
#endif
}

void Slake::Server::deinit() {
#ifdef _WIN32
	WSACleanup();
#endif
}
