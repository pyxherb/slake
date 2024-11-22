#ifndef _SLAKE_UTIL_STRING_H_
#define _SLAKE_UTIL_STRING_H_

#include <memory_resource>
#include <cstring>
#include <stdexcept>

namespace slake {
	namespace util {
		class ANSIString final {
		public:
			std::pmr::memory_resource *memoryResource;
			char *data = nullptr;
			size_t length = 0;

			ANSIString(std::pmr::memory_resource *memoryResource) noexcept;
			ANSIString(std::pmr::memory_resource *memoryResource, size_t len, char c);
			ANSIString(std::pmr::memory_resource *memoryResource, const char *s);
			ANSIString(const ANSIString &other);
			ANSIString(ANSIString &&other) noexcept;

			void clear() noexcept;
			void discardAndResize(size_t newSize);
			void resizeUninitialized(size_t newSize);
			void resize(size_t newSize);

			char &at(size_t index);
			const char &at(size_t index) const;

			ANSIString substr(size_t index, size_t length) const;

			void set(const char *s);
		};
	}
}

#endif
