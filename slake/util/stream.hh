#ifndef _SLAKE_UTIL_STREAM_HH_
#define _SLAKE_UTIL_STREAM_HH_

#include <cstdint>
#include <istream>

namespace slake {
	namespace util {
		class InputMemStream : virtual private std::streambuf,
							   virtual public std::istream {
		private:
			const void *src;
			size_t _size;

		public:
			inline InputMemStream(const void *src, std::streamsize size)
				: basic_istream(this),
				  src(src),
				  _size(size) {
				setbuf((char *)src, size);
			}
			virtual ~InputMemStream() = default;
		};
	}
}

#endif
