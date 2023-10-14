#ifndef _SLAKE_REG_H_
#define _SLAKE_REG_H_

#include <cstdint>

namespace slake {
	enum class RegId : uint8_t {
		TMP0 = 0,  // Temporary register #0
		R0,		   // General-purposed register #0
		R1,		   // General-purposed register #1
		R2,		   // General-purposed register #2
		RR,		   // Result register
		RTHIS,	   // This register
		RXCPT,	   // Exception register
	};
}

#endif
