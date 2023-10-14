#ifndef _SLKC_COMPILER_AST_TYPENAME_H_
#define _SLKC_COMPILER_AST_TYPENAME_H_

#include "ref.h"

namespace slake {
	namespace slkc {
		class TypeNameNode : public AstNode {
		private:
			Location _loc;

		public:
			string content;

			inline TypeNameNode(string content) : content(content) {}
			virtual ~TypeNameNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_COMMENT; }
		};
	}
}

namespace std {
	string to_string(shared_ptr<slake::slkc::TypeNameNode> typeName);
}

#endif
