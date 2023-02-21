#ifndef _SLKC_LSP_NET_HH
#define _SLKC_LSP_NET_HH

#include <stdexcept>

namespace Slake {
	namespace Server {
		class Socket {
		protected:
			int _socket;

		public:

		};

		void init();
		void deinit();
	}
}

#endif
