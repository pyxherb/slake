#include "net.h"

#ifdef _WIN32
	#include <WinSock2.h>
#elif __linux__
	#include <sys/socket.h>
#endif

void slake::Server::init() {
#ifdef _WIN32
	{
		WSADATA wsaData;
		if (!WSAStartup(MAKEWORD(2, 2), &wsaData))
			throw std::runtime_error("Error initializing WSA");
	}
#elif __linux__
#else
	throw std::logic_error("LSP server is not implemented yet");
#endif
}

void slake::Server::deinit() {
#ifdef _WIN32
	WSACleanup();
#elif __linux__
#endif
}
