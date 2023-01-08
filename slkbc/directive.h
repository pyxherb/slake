#ifndef _SLKBC_DIRECTIVE_H_
#define _SLKBC_DIRECTIVE_H_

#include <functional>

struct DirectiveRegistry {
	const char* name;
	std::function<> proc;
};

#endif
