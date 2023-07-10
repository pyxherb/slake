#ifndef _SLKC_COMPILER_NODE_H_
#define _SLKC_COMPILER_NODE_H_

#include <deque>
#include <memory>
#include <unordered_map>
#include <antlr4-runtime.h>

namespace slake {
	namespace slkc {
		using namespace std;

		class AstNode {
		public:
			virtual ~AstNode() = default;
		};
	}
}

#endif
