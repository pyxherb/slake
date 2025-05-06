#ifndef _SLAKE_LOADER_READER_H_
#define _SLAKE_LOADER_READER_H_

#include <slake/runtime.h>

namespace slake {
	namespace loader {
		class Reader {
		public:
			SLAKE_API virtual ~Reader();

			[[nodiscard]] virtual bool read(char *buffer, size_t size) noexcept = 0;

			[[nodiscard]] SLAKE_FORCEINLINE bool readI8(int8_t &data) noexcept {
				return read((char *)&data, sizeof(int8_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readI16(int16_t &data) noexcept {
				return read((char *)&data, sizeof(int16_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readI32(int32_t &data) noexcept {
				return read((char *)&data, sizeof(int32_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readI64(int64_t &data) noexcept {
				return read((char *)&data, sizeof(int64_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readU8(uint8_t &&data) noexcept {
				return read((char *)&data, sizeof(uint8_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readU16(uint16_t &data) noexcept {
				return read((char *)&data, sizeof(uint16_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readU32(int32_t &data) noexcept {
				return read((char *)&data, sizeof(uint32_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readU64(uint64_t &data) noexcept {
				return read((char *)&data, sizeof(uint64_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readBool(bool &data) noexcept {
				return read((char *)&data, sizeof(bool));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readF32(float &data) noexcept {
				return read((char *)&data, sizeof(float));
			}

			[[nodiscard]] SLAKE_FORCEINLINE bool readF64(double &data) noexcept {
				return read((char *)&data, sizeof(double));
			}
		};
	}
}

#endif
