#ifndef _SPKC_SYNTAX_TYPENAME_HH
#define _SPKC_SYNTAX_TYPENAME_HH

#include "expr.hh"

namespace SpkC {
	namespace Syntax {
		enum class EvalType : int {
			NONE = 0,
			I8,
			I16,
			I32,
			I64,
			U8,
			U16,
			U32,
			U64,
			FLOAT,
			DOUBLE,
			STRING,
			AUTO,
			ARRAY,
			MAP,
			CUSTOM
		};
		class TypeName {
		public:
			EvalType typeName = EvalType::NONE;

			inline TypeName(EvalType typeName) {
				this->typeName = typeName;
			}
			virtual inline ~TypeName() {}

			virtual bool operator==(const TypeName& x) const {
				return typeName == x.typeName;
			}
		};

		class CustomTypeName : public TypeName {
		public:
			std::shared_ptr<RefExpr> typeRef = nullptr;

			inline CustomTypeName(std::shared_ptr<RefExpr> typeRef) : TypeName(EvalType::CUSTOM) {
				this->typeRef = typeRef;
			}
		};
	}
}

#endif
