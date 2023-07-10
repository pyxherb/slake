#ifndef _SLAKE_UTIL_ANY_HH_
#define _SLAKE_UTIL_ANY_HH_

#include <cstdint>
#include <functional>
#include <type_traits>

namespace slake {
	namespace util {
		class Any {
		protected:
			void *_data;
			std::function<void()> _destructor = []() {};

		public:
			template <typename T>
			inline Any &operator=(T &&data) {
				_destructor();

				if constexpr (std::is_destructible<T>::value) {
					_destructor = [this]() {
						delete (T *)_data;
					};
				} else
					_destructor = []() {};

				_data = new char[sizeof(T)];
				*((T *)_data) = data;
			}

			template <typename T>
			inline Any &operator=(T &data) {
				return *this = std::move(data);
			}

			template <typename T>
			inline Any(T& data) {
				*this = data;
			}
			template <typename T>
			inline Any(T &&data) {
				*this = data;
			}

			inline ~Any() {
				_destructor();
			}

			template <typename T>
			const T& get() const {
				return *(T*)_data;
			}

			template <typename T>
			T &get() {
				return *(T *)_data;
			}
		};
	}
}

#endif
