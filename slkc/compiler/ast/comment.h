#ifndef _SLKC_COMPILER_AST_COMMENT_H_
#define _SLKC_COMPILER_AST_COMMENT_H_

#include "idref.h"

namespace slake {
	namespace slkc {
		class TypeNameNode : public AstNode {
		private:
			Location _loc;

		public:
			std::string content;

			inline TypeNameNode(std::string content) : content(content) {}
			virtual ~TypeNameNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Comment; }
		};
	}
}

namespace std {
	std::string to_string(std::shared_ptr<slake::slkc::TypeNameNode> typeName);
}

#endif
