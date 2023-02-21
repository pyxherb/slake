#ifndef _SLKC_COMPILER_TYPENAME_HH
#define _SLKC_COMPILER_TYPENAME_HH

#include <vector>

#include "base.hh"

namespace Slake {
	namespace Compiler {
		enum class EvalType : int {
			NONE = 0,
			ANY,
			I8,
			I16,
			I32,
			ISIZE = I32,
			I64,
			U8,
			U16,
			U32,
			USIZE = U32,
			U64,
			FLOAT,
			DOUBLE,
			STRING,
			ARRAY,
			BOOL,
			MAP,
			FN,
			AUTO,
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
					case EvalType::STRING:
						return "string";
					case EvalType::BOOL:
						return "bool";
					case EvalType::AUTO:
						return "auto";
					case EvalType::ANY:
						return "any";
					case EvalType::ARRAY:
						return "(Array)";
					case EvalType::MAP:
						return "(Map)";
					case EvalType::FN:
						return "(Function)";
					case EvalType::CUSTOM:
						return "(User-defined)";
				}
				return "(Unknwon type)";
			}
		};

		class RefExpr;
		class Scope;
		class CustomTypeName : public TypeName {
		public:
			std::shared_ptr<RefExpr> typeRef;
			std::weak_ptr<Scope> scope;	 // Scope where the type name constructed

			inline CustomTypeName(location loc, std::shared_ptr<RefExpr> typeRef, std::shared_ptr<Scope> scope) : TypeName(loc, EvalType::CUSTOM) {
				this->typeRef = typeRef;
				this->scope = scope;
			}
			virtual inline ~CustomTypeName() {}

			virtual std::string toString() const override;
		};

		class ArrayTypeName : public TypeName {
		public:
			std::shared_ptr<TypeName> type;

			inline ArrayTypeName(location loc, std::shared_ptr<TypeName> type) : TypeName(loc, EvalType::ARRAY) {
				this->type = type;
			}
			virtual inline ~ArrayTypeName() {}

			virtual inline std::string toString() const override {
				return std::to_string(*type) + "[]";
			}
		};

		class FnTypeName : public TypeName {
		public:
			std::shared_ptr<TypeName> resultType;
			std::vector<std::shared_ptr<TypeName>> argTypes;
			Base::UUID uuid;

			inline FnTypeName(location loc, std::shared_ptr<TypeName> resultType, Base::UUID uuid = Base::UUID()) : TypeName(loc, EvalType::FN) {
				this->resultType = resultType;
				this->uuid = uuid;
			}

			virtual inline ~FnTypeName() {}

			virtual inline std::string toString() const override {
				return "fn" + std::to_string(*resultType);
			}

			inline bool isNative() noexcept {
				return uuid;
			}
		};

		bool isSameType(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
		bool isConvertible(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
		bool isBaseOf(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
		bool isDerivedFrom(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
	}
}

#endif
