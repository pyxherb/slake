
#ifndef _SLAKE_BASE_UUID_DEFAULT_HH_
#define _SLAKE_BASE_UUID_DEFAULT_HH_

#include <cstdint>
#include <random>

namespace Slake {
	namespace Base {
		namespace Details {
			///
			/// @brief Get random generated MAC address.
			///
			/// @param node Node member of the UUID.
			///
			void uuidGetMacAddr(std::uint8_t node[6]) {
				auto gen = std::random_device();
				// If not, generate a random sequence instead
				*(std::uint32_t*)node = gen();
				*(std::uint32_t*)&(node[4]) = gen() & 0xffff;
			}
		}
	}
}

#endif
