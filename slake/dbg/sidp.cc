#include "sidp.h"
#ifdef _WIN32
	#include <WinSock2.h>
	#include <Windows.h>
typedef int socklen_t;
#else
	#include <sys/fcntl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
typedef int SOCKET;
#endif

#include <stdexcept>
#include <string>

using namespace slake;

void SidpDebugAdapter::_sidpListenerThreadProc(SidpDebugAdapter *adapter, uint16_t port) {
	SOCKET s;
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		throw std::runtime_error("Error creating socket");

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = port;

	if (bind(s, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 1)
		throw std::runtime_error("Error binding SDIP socket with port " + std::to_string(port));

	if (listen(s, 1) < 1)
		throw std::runtime_error("Error listening with the SDIP socket");

	sockaddr clientAddr;
	socklen_t szClientAddr = sizeof(clientAddr);
	while (!adapter->_exit) {
		if (!accept(s, &clientAddr, &szClientAddr))
			std::this_thread::yield();

		// Verify the client version.
		{
			uint8_t clientVer[2];

			if (recv(s, (char*)clientVer, sizeof(clientVer), MSG_WAITALL) != sizeof(clientVer))
				return;
			if (clientVer[0] != SIDP_VERSION_MAJOR)
				continue;
		}

		send(s, ACCEPTED_MAGIC, sizeof(ACCEPTED_MAGIC), 0);

		// Verify data of the `accepted packet'.
		{
			char buf[sizeof(ACCEPTED_MAGIC)];
			if (recv(s, buf, sizeof(buf), MSG_WAITALL) != sizeof(buf))
				return;
			if (memcmp(buf, ACCEPTED_MAGIC, sizeof(ACCEPTED_MAGIC)))
				return;
		}

		while (true) {

		}
	}
}

SidpDebugAdapter::SidpDebugAdapter(uint16_t port) {
	std::thread t(_sidpListenerThreadProc, this, port);
	t.detach();
}

SidpDebugAdapter::~SidpDebugAdapter() {
	_exit = true;
	while (_exit)
		std::this_thread::yield();
}

void SidpDebugAdapter::onExecBreakpoint(FnObject* fn, uint32_t curIns) {

}

void SidpDebugAdapter::onVarWrite(RegularVarObject *var) {

}
