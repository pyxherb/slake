#ifndef _SLAKE_UTIL_MEMORY_H_
#define _SLAKE_UTIL_MEMORY_H_

#include <type_traits>

namespace slake {
	namespace util {
		template <typename T, typename V = void>
		struct IsDeallocable : std::false_type {
		};

		template <typename T>
		struct IsDeallocable<T, std::void_t<decltype(std::declval<T>().dealloc())>> : std::true_type {
		};

		template <typename T>
		struct DeallocableDeleter {
			static_assert(IsDeallocable<T>::value, "Specified type is not deallocatable");
			void operator()(T *ptr) {
				if (ptr)
					ptr->dealloc();
			}
		};

		template <typename Alloc>
		struct StatefulDeleter {
			Alloc allocator;

			StatefulDeleter(const Alloc &allocator) : allocator(allocator) {
			}
			StatefulDeleter(Alloc &&allocator) : allocator(allocator) {
			}
			void operator()(typename Alloc::value_type *ptr) {
				if (ptr)
					allocator.deallocate(ptr, 1);
			}
		};
	}
}

#endif
