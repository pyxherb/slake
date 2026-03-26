#ifndef _SLAKE_LOADER_READER_H_
#define _SLAKE_LOADER_READER_H_

#include <slake/runtime.h>

namespace slake {
	namespace loader {
		enum class ReadResult {
			Succeeded = 0,
			ReadError
		};

		class Reader {
		public:
			SLAKE_API virtual ~Reader();

			[[nodiscard]] virtual bool is_eof() noexcept = 0;
			[[nodiscard]] virtual ReadResult read(char *buffer, size_t size) noexcept = 0;

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_i8(int8_t &data) noexcept {
				return read((char *)&data, sizeof(int8_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_i16(int16_t &data) noexcept {
				return read((char *)&data, sizeof(int16_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_i32(int32_t &data) noexcept {
				return read((char *)&data, sizeof(int32_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_i64(int64_t &data) noexcept {
				return read((char *)&data, sizeof(int64_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_u8(uint8_t &data) noexcept {
				return read((char *)&data, sizeof(uint8_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_u16(uint16_t &data) noexcept {
				return read((char *)&data, sizeof(uint16_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_u32(uint32_t &data) noexcept {
				return read((char *)&data, sizeof(uint32_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_u64(uint64_t &data) noexcept {
				return read((char *)&data, sizeof(uint64_t));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_bool(bool &data) noexcept {
				return read((char *)&data, sizeof(bool));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_f32(float &data) noexcept {
				return read((char *)&data, sizeof(float));
			}

			[[nodiscard]] SLAKE_FORCEINLINE ReadResult read_f64(double &data) noexcept {
				return read((char *)&data, sizeof(double));
			}

			virtual void dealloc() noexcept = 0;
		};
	}
}

#endif
