#ifndef _SLKC_COMPILER_TYPENAME_HH
#define _SLKC_COMPILER_TYPENAME_HH

#include "expr.hh"

namespace Slake {
	namespace Compiler {
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
		class TypeName : public BasicLocated,
						 public IStringifiable {
		public:
			EvalType typeName = EvalType::NONE;

			inline TypeName(location loc, EvalType typeName) : BasicLocated(loc) {
				this->typeName = typeName;
			}
			virtual inline ~TypeName() {}

			virtual bool operator==(const TypeName& x) const {
				return typeName == x.typeName;
			}

			virtual inline std::string toString() const override {
				switch (typeName) {
					case EvalType::NONE:
						return "void";
					case EvalType::I8:
						return "i8";
					case EvalType::I16:
						return "i16";
					case EvalType::I32:
						return "i32";
					case EvalType::I64:
						return "i64";
					case EvalType::U8:
						return "u8";
					case EvalType::U16:
						return "u16";
					case EvalType::U32:
						return "u32";
					case EvalType::U64:
						return "u64";
					case EvalType::FLOAT:
						return "float";
					case EvalType::DOUBLE:
						return "double";
					case EvalType::AUTO:
						return "auto";
					case EvalType::ARRAY:
						return "(Array)";
					case EvalType::MAP:
						return "(Map)";
					case EvalType::CUSTOM:
						return "(User-defined)";
				}
				return "(Unknwon type)";
			}
		};

		class CustomTypeName : public TypeName {
		public:
			std::shared_ptr<RefExpr> typeRef = nullptr;

			inline CustomTypeName(location loc, std::shared_ptr<RefExpr> typeRef) : TypeName(loc, EvalType::CUSTOM) {
				this->typeRef = typeRef;
			}

			virtual inline std::string toString() const override {
				return "@" + std::to_string(*typeRef);
			}
		};
	}
}

#endif
