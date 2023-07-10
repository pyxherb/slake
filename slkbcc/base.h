#ifndef _SLKBCC_BASE_H_
#define _SLKBCC_BASE_H_

#include <location.hh>
#include <slake/access.h>

namespace slake {
	namespace bcc {
		using namespace std;

		class ILocated {
		public:
			virtual ~ILocated() = default;

			virtual location getLocation() const = 0;
		};
	}
}

#endif
