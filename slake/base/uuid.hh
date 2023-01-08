///
/// @file uuid.hh
/// @author CodesBuilder (2602783536@qq.com)
/// @brief A simple UUID implementation.
/// @version 0.1
/// @date 2022-10-01
///
/// @copyright Copyright (c) 2022 Slake Contributors
///
#ifndef _SLAKE_UUID_H_
#define _SLAKE_UUID_H_

#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>

#ifdef _WIN32
	#include "uuid/win32.hh"
#else
	#include "uuid/default.hh"
#endif

namespace Slake {
	namespace Base {
		class UUID final {
		public:
			std::uint32_t timeLow;

			std::uint16_t timeMid;
			std::uint16_t timeHiAndVer;

			std::uint8_t clockSeqHiAndReserved, clockSeqLow;

			std::uint8_t node[6];

			inline bool operator==(UUID& x) {
				return (timeLow == x.timeLow) &&
					   (timeMid == x.timeMid) &&
					   (timeHiAndVer == x.timeHiAndVer) &&
					   (clockSeqHiAndReserved == x.clockSeqHiAndReserved) &&
					   (clockSeqLow == x.clockSeqLow) &&
					   ((*(std::uint32_t*)node) == (*(std::uint32_t*)x.node)) &&
					   ((*(std::uint32_t*)(node + 4)) == (*(std::uint32_t*)(x.node + 4)));
			}

			inline bool operator<(UUID& x) {
				if (timeLow >= x.timeLow)
					return false;
				if (timeMid >= x.timeMid)
					return false;
				if (timeHiAndVer >= x.timeHiAndVer)
					return false;
				if (clockSeqHiAndReserved >= x.clockSeqHiAndReserved)
					return false;
				if (clockSeqLow >= x.clockSeqLow)
					return false;
				if (((*(std::uint32_t*)node) >= (*(std::uint32_t*)x.node)))
					return false;
				if (((*(std::uint32_t*)(node + 4)) >= (*(std::uint32_t*)(x.node + 4))))
					return false;
				return true;
			}

			inline bool operator>(UUID& x) {
				if (timeLow <= x.timeLow)
					return false;
				if (timeMid <= x.timeMid)
					return false;
				if (timeHiAndVer <= x.timeHiAndVer)
					return false;
				if (clockSeqHiAndReserved <= x.clockSeqHiAndReserved)
					return false;
				if (clockSeqLow <= x.clockSeqLow)
					return false;
				if (((*(std::uint32_t*)node) <= (*(std::uint32_t*)x.node)))
					return false;
				if (((*(std::uint32_t*)(node + 4)) <= (*(std::uint32_t*)(x.node + 4))))
					return false;
				return true;
			}

			static inline UUID parse(std::string s) {
				UUID uuid;

				// 8-4-4-4-12
				if (s.length() == 38)
					s = s.substr(1, 36);
				if (s.length() != 36)
					throw std::invalid_argument("Error parsing UUID");

				if (s[8] != '-' ||
					s[13] != '-' ||
					s[18] != '-' ||
					s[23] != '-')
					throw std::invalid_argument("Error parsing UUID");

				char* ptr = nullptr;
				uuid.timeLow = (std::uint32_t)std::strtoul(s.substr(0, 8).c_str(), &ptr, 16);
				if (*ptr)
					throw std::invalid_argument("Error parsing UUID");

				ptr = nullptr;
				uuid.timeMid = (std::uint16_t)std::strtoul(s.substr(9, 4).c_str(), &ptr, 16);
				if (*ptr)
					throw std::invalid_argument("Error parsing UUID");

				ptr = nullptr;
				uuid.timeHiAndVer = (std::uint16_t)std::strtoul(s.substr(14, 4).c_str(), &ptr, 16);
				if (*ptr)
					throw std::invalid_argument("Error parsing UUID");

				ptr = nullptr;
				uuid.clockSeqHiAndReserved = (std::uint8_t)std::strtoul(s.substr(19, 2).c_str(), &ptr, 16);
				if (*ptr)
					throw std::invalid_argument("Error parsing UUID");

				ptr = nullptr;
				uuid.clockSeqLow = (std::uint8_t)std::strtoul(s.substr(21, 2).c_str(), &ptr, 16);
				if (*ptr)
					throw std::invalid_argument("Error parsing UUID");

				for (std::size_t i = 0; i < 6; i++) {
					ptr = nullptr;
					uuid.node[i] = (std::uint32_t)std::strtoul(s.substr(24 + 2 * i, 2).c_str(), &ptr, 16);
					if (*ptr)
						throw std::invalid_argument("Error parsing UUID");
				}

				return uuid;
			}

			static inline UUID uuid1() {
				UUID uuid{};

				auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
				static uint64_t clockSeq = UINT64_MAX;
				if (clockSeq == UINT64_MAX)
					clockSeq = std::mt19937_64(now)();
				else
					clockSeq++;

				uuid.timeLow = (uint32_t)(now & 0xffffffff);
				uuid.timeMid = (uint16_t)((now >> 32) & 0xffff);
				uuid.timeHiAndVer = (uint16_t)(((now >> 48) & 0xfff) | (1 << 12));
				uuid.clockSeqLow = (uint8_t)(clockSeq & 0xff);
				uuid.clockSeqHiAndReserved = (uint8_t)(((clockSeq & 0x3f00) >> 8) | 0x80);

				// Get MAC addresses from network adapters.
				Details::uuidGetMacAddr(uuid.node);

				return uuid;
			}

			static inline UUID uuid2();
			static inline UUID uuid3();
			static inline UUID uuid4();
			static inline UUID uuid5();

			inline UUID& operator=(UUID& x) {
				timeLow=x.timeLow;
				timeMid=x.timeMid;
				timeHiAndVer=x.timeHiAndVer;
				clockSeqHiAndReserved=x.clockSeqHiAndReserved;
				clockSeqLow=x.clockSeqLow;
				std::memcpy(node,x.node,sizeof(node));
			}
		};
	}
}

namespace std {
	std::string to_string(Slake::Base::UUID& uuid) {
		char s[37];
		sprintf(
			s,
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			uuid.timeLow,
			uuid.timeMid,
			uuid.timeHiAndVer,
			uuid.clockSeqHiAndReserved,
			uuid.clockSeqLow,
			uuid.node[0], uuid.node[1], uuid.node[2], uuid.node[3], uuid.node[4], uuid.node[5]);
		return std::string(s);
	}
}

#endif
