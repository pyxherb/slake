///
/// @file win32.hh
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Native UUID implementations for Win32 platforms.
/// @version 0.1
/// @date 2022-10-01
///
/// @copyright Copyright (c) 2022 Slake Contributors
///
#ifndef _SLAKE_BASE_UUID_WIN32_HH_
#define _SLAKE_BASE_UUID_WIN32_HH_

#include <Windows.h>
#include <iphlpapi.h>

#include <cstdint>
#include <random>
#pragma comment(lib, "iphlpapi.lib")

namespace Slake {
	namespace Base {
		namespace Details {
			///
			/// @brief Get MAC address.
			///
			/// @param node Node member of the UUID.
			///
			inline void uuidGetMacAddr(std::uint8_t node[6]) {
				PIP_ADAPTER_INFO adapterInfo = new IP_ADAPTER_INFO();
				ULONG size = sizeof(IP_ADAPTER_INFO), ret;

				if ((ret = GetAdaptersInfo(adapterInfo, &size)) == ERROR_BUFFER_OVERFLOW) {
					delete adapterInfo;
					adapterInfo = (PIP_ADAPTER_INFO) new BYTE[size];
					ret = GetAdaptersInfo(adapterInfo, &size);
				}

				// Check if we've got adapter information successfully
				if (SUCCEEDED(ret)) {
					PIP_ADAPTER_INFO info = adapterInfo;

					// Enumerate for each adapters
					while (info) {
						// Check if it is an ethernet adapter and its MAC address is valid
						if ((info->Type == MIB_IF_TYPE_ETHERNET || info->Type == IF_TYPE_IEEE80211) && info->AddressLength > 0) {
							// Copy the MAC address.
							memset(node, 0, 6);
							memcpy(node, info->Address, info->AddressLength > 6 ? 6 : info->AddressLength);
							break;
						}
						info = info->Next;
					}
				} else {
					// If not, generate a random number instead
					*(uint32_t*)node = std::random_device()();
					*(uint16_t*)&(node[4]) = std::random_device()() & 0xffff;
				}

				delete adapterInfo;
			}
		}
	}
}

#endif
