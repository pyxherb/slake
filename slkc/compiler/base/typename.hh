#ifndef _SLKC_COMPILER_TYPENAME_HH
#define _SLKC_COMPILER_TYPENAME_HH

#include <vector>

#include "misc.hh"

namespace Slake {
	namespace Compiler {
		enum class TypeNameKind : int {
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
			TypeNameKind kind = TypeNameKind::NONE;

			inline TypeName(location loc, TypeNameKind kind) : BasicLocated(loc) {
				this->kind = kind;
			}
			virtual inline ~TypeName() {}

			virtual bool operator==(const TypeName& x) const {
				return kind == x.kind;
			}

			virtual inline std::string toString() const override {
				switch (kind) {
					case TypeNameKind::NONE:
						return "void";
					case TypeNameKind::I8:
						return "i8";
					case TypeNameKind::I16:
						return "i16";
					case TypeNameKind::I32:
						return "i32";
					case TypeNameKind::I64:
						return "i64";
					case TypeNameKind::U8:
						return "u8";
					case TypeNameKind::U16:
						return "u16";
					case TypeNameKind::U32:
						return "u32";
					case TypeNameKind::U64:
						return "u64";
					case TypeNameKind::FLOAT:
						return "float";
					case TypeNameKind::DOUBLE:
						return "double";
					case TypeNameKind::STRING:
						return "string";
					case TypeNameKind::BOOL:
						return "bool";
					case TypeNameKind::AUTO:
						return "auto";
					case TypeNameKind::ANY:
						return "any";
					case TypeNameKind::ARRAY:
						return "(Array)";
					case TypeNameKind::MAP:
						return "(Map)";
					case TypeNameKind::FN:
						return "(Function)";
					case TypeNameKind::CUSTOM:
						return "(User-defined)";
				}
				return "(Unknwon type)";
			}
		};

		class RefExpr;
		class Scope;
		class Type;
		class CustomTypeName : public TypeName {
		protected:
			std::weak_ptr<Type> _cachedType;

		public:
			std::shared_ptr<RefExpr> typeRef;
			std::weak_ptr<Scope> scope;	 // Scope where the type name constructed

			inline CustomTypeName(location loc, std::shared_ptr<RefExpr> typeRef, std::shared_ptr<Scope> scope) : TypeName(loc, TypeNameKind::CUSTOM) {
				this->typeRef = typeRef;
				this->scope = scope;
			}
			virtual inline ~CustomTypeName() {}

			std::shared_ptr<Type> resolveType();

			virtual std::string toString() const override;
		};

		class ArrayTypeName : public TypeName {
		public:
			std::shared_ptr<TypeName> type;

			inline ArrayTypeName(location loc, std::shared_ptr<TypeName> type) : TypeName(loc, TypeNameKind::ARRAY) {
				this->type = type;
			}
			virtual inline ~ArrayTypeName() {}

			virtual inline std::string toString() const override {
				return std::to_string(*type) + "[]";
			}
		};

		class MapTypeName : public TypeName {
		public:
			std::shared_ptr<TypeName> keyType, valueType;

			inline MapTypeName(location loc, std::shared_ptr<TypeName> keyType, std::shared_ptr<TypeName> valueType)
				: TypeName(loc, TypeNameKind::MAP), keyType(keyType), valueType(valueType) {
			}
			virtual inline ~MapTypeName() {}

			virtual inline std::string toString() const override {
				return std::to_string(*valueType) + "[" + std::to_string(*keyType) + "]";
			}
		};

		class FnTypeName : public TypeName {
		public:
			std::shared_ptr<TypeName> resultType;
			std::vector<std::shared_ptr<TypeName>> argTypes;

			inline FnTypeName(location loc, std::shared_ptr<TypeName> resultType) : TypeName(loc, TypeNameKind::FN), resultType(resultType) {}
			virtual inline ~FnTypeName() {}

			virtual inline std::string toString() const override {
				return "fn" + std::to_string(*resultType);
			}
		};

		bool isSameType(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
		bool isConvertible(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
		bool isBaseOf(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
		bool isDerivedFrom(std::shared_ptr<TypeName> t1, std::shared_ptr<TypeName> t2);
	}
}

#endif
