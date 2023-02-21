#ifndef _SLKC_COMPILER_SCOPE_HH
#define _SLKC_COMPILER_SCOPE_HH

#include <algorithm>
#include <map>
#include <unordered_map>

#include "stmt.hh"

namespace Slake {
	namespace Compiler {
		class Scope;

		class ParamDecl final : public BasicLocated,
								public IStringifiable {
		public:
			std::string name;
			std::shared_ptr<TypeName> typeName;
			std::shared_ptr<Expr> initValue;

			ParamDecl(location loc,
				std::string name,
				std::shared_ptr<TypeName> typeName,
				std::shared_ptr<Expr> initValue = std::shared_ptr<Expr>()) : BasicLocated(loc) {
				this->name = name;
				this->typeName = typeName;
				this->initValue = initValue;
			}
			inline ~ParamDecl() {}

			virtual inline std::string toString() const override {
				if (name == "...")
					return "...";
				std::string s = std::to_string(*typeName) + " " + name;
				if (initValue)
					s += " = " + std::to_string(*initValue);
				return s;
			}
		};

		class ParamDeclList final : public ILocated,
									public IStringifiable,
									public std::vector<std::shared_ptr<ParamDecl>> {
		public:
			inline ParamDeclList() {}
			inline ~ParamDeclList() {}

			std::shared_ptr<ParamDecl> operator[](std::string name) {
				for (auto& i : *this) {
					if (i->name == name)
						return i;
				}
				return std::shared_ptr<ParamDecl>();
			}

			virtual location getLocation() const override { return this->at(0)->getLocation(); }

			virtual inline std::string toString() const override {
				std::string s;

				auto i = this->begin();
				if (i != this->end())
					s = std::to_string((**i++));
				while (i != this->end())
					s += ", " + std::to_string((**i++));

				return s;
			}
		};

		class FnDef : public IAccessModified,
					  public BasicLocated,
					  public IStringifiable {
		public:
			std::shared_ptr<ParamDeclList> params;
			std::shared_ptr<TypeName> returnTypeName;
			std::shared_ptr<CodeBlock> execBlock;
			Base::UUID uuid;
			std::string name;

			inline FnDef(
				location loc,
				AccessModifier accessModifier,
				std::shared_ptr<ParamDeclList> params,
				std::shared_ptr<TypeName> returnTypeName,
				std::shared_ptr<CodeBlock> execBlock = std::shared_ptr<CodeBlock>(),
				std::string name = "(Anonymous)") : IAccessModified(accessModifier), BasicLocated(loc) {
				this->params = params;
				this->returnTypeName = returnTypeName;
				this->execBlock = execBlock;
				this->name = name;
			}
			inline FnDef(
				location loc,
				AccessModifier accessModifier,
				std::shared_ptr<ParamDeclList> params,
				std::shared_ptr<TypeName> returnTypeName,
				Base::UUID uuid,
				std::string name = "(Anonymous)") : IAccessModified(accessModifier), BasicLocated(loc) {
				this->params = params;
				this->returnTypeName = returnTypeName;
				this->uuid = uuid;
				this->name = name;
			}
			virtual inline ~FnDef() {}

			virtual inline std::string toString() const override {
				std::string s = genIndentStr() + (returnTypeName ? std::to_string(*returnTypeName) + " " : "") + name + "(" + std::to_string(*params) + ")";
				s += execBlock ? std::to_string(*execBlock) + "\n" : ";\n";
				return s;
			}
			inline bool isNative() {
				return uuid;
			}
		};

		struct ImportItem final : public BasicLocated {
		public:
			std::shared_ptr<RefExpr> path;

			inline ImportItem(location loc, std::shared_ptr<RefExpr> path, bool searchInSystemPath = false) : BasicLocated(loc) {
				this->path = path;
			}
			inline ~ImportItem() {}
		};

		struct VarDefItem final : public BasicLocated, public IAccessModified {
			std::shared_ptr<TypeName> typeName;
			std::shared_ptr<Expr> initValue;

			inline VarDefItem(location loc, AccessModifier accessModifier, std::shared_ptr<TypeName> typeName, std::shared_ptr<Expr> initValue) : BasicLocated(loc), IAccessModified(accessModifier) {
				this->typeName = typeName;
				this->initValue = initValue;
			}
		};

		struct ImplList final : public ILocated,
								public IStringifiable {
			std::vector<std::shared_ptr<TypeName>> impls;

			virtual location getLocation() const override { return impls[0]->getLocation(); }

			virtual inline std::string toString() const override {
				if (!impls.size())
					return "";
				std::string s;

				auto i = impls.begin();
				s = std::to_string((**i++));
				while (i != impls.end())
					s += ", " + std::to_string((**i++));

				return s;
			}
		};

		class Type : public IAccessModified,
					 public IStringifiable,
					 public BasicLocated {
		public:
			enum class Kind : std::uint8_t {
				CLASS = 0,
				TRAIT,
				ENUM,
				STRUCT
			};
			inline Type(location loc, AccessModifier accessModifier) : IAccessModified(accessModifier), BasicLocated(loc) {}
			virtual inline ~Type() {}
			virtual Kind getKind() = 0;

			virtual inline std::shared_ptr<Scope> getScope() const {
				return std::shared_ptr<Scope>();
			}
		};

		template <Type::Kind kind>
		class BasicClassType final : public Type {
		public:
			std::shared_ptr<Scope> scope;
			std::shared_ptr<TypeName> parent;
			std::shared_ptr<ImplList> impls;
			std::vector<std::string> genericParams;
			std::string name;

			inline BasicClassType(
				location loc,
				AccessModifier accessModifier,
				std::shared_ptr<Scope> scope,
				std::shared_ptr<TypeName> parent,
				std::string name = "(Anonymous)",
				std::vector<std::string> genericParams = {},
				std::shared_ptr<ImplList> impls = std::shared_ptr<ImplList>()) : Type(loc, accessModifier) {
				this->scope = scope;
				this->parent = parent;
				this->impls = impls;
				this->genericParams = genericParams;
				this->name = name;
			}
			virtual inline ~BasicClassType() {}

			virtual Kind getKind() override { return kind; }

			virtual inline std::string toString() const override {
				std::string s = genIndentStr();

				switch (kind) {
					case Kind::CLASS:
						s += "class ";
						break;
					case Kind::TRAIT:
						s += "trait ";
						break;
					default:
						s += "(Class Type) ";
				}
				s += name;
				if (genericParams.size()) {
					s += "<";

					auto i = genericParams.begin();
					s += *i++;
					while (i != genericParams.end())
						s += ", " + *i++;

					s += ">";
				}
				if (parent)
					s += "(" + std::to_string(*parent) + ")";
				if (impls)
					s += " : " + std::to_string(*impls);

				indentLevel++;
				s += " {\n" + std::to_string(*scope) + "}";
				indentLevel--;

				return s;
			}

			virtual inline std::shared_ptr<Scope> getScope() const override {
				return scope;
			}
		};

		using ClassType = BasicClassType<Type::Kind::CLASS>;
		using TraitType = BasicClassType<Type::Kind::TRAIT>;

		class EnumType final : public Type {
		public:
			std::shared_ptr<TypeName> typeName;
			std::map<std::string, std::shared_ptr<Expr>> pairs;
			std::string name;

			inline EnumType(location loc, AccessModifier accessModifier, std::string name, std::shared_ptr<TypeName> typeName) : Type(loc, accessModifier) {
				this->typeName = typeName;
				this->name = name;
			}
			virtual inline ~EnumType() {}

			virtual Kind getKind() override { return Kind::ENUM; }

			virtual inline std::string toString() const override {
				std::string s = genIndentStr() + "enum : " + std::to_string(*typeName) + " {\n";

				indentLevel++;
				for (auto i : pairs)
					s += genIndentStr() + i.first + " = " + std::to_string(*(i.second)) + ",\n";
				indentLevel--;

				s += genIndentStr() + "}";

				return s;
			}
		};

		class StructType final : public Type {
		private:
			std::unordered_map<std::string, std::size_t> _varIndices;
			std::vector<std::shared_ptr<VarDefItem>> _vars;

		public:
			std::string name;

			inline StructType(location loc, AccessModifier accessModifier, std::string name) : Type(loc, accessModifier) {
				this->name = name;
			}
			virtual inline ~StructType() {}

			virtual inline Kind getKind() override { return Kind::STRUCT; }

			inline std::shared_ptr<VarDefItem> getMember(std::string name) {
				if (!_varIndices.count(name))
					return std::shared_ptr<VarDefItem>();
				return _vars[_varIndices[name]];
			}
			void addMembers(std::shared_ptr<VarDefStmt> varDecls);
			inline const std::vector<std::shared_ptr<VarDefItem>>& getMembers() { return _vars; }

			virtual inline std::string toString() const override {
				std::string s = genIndentStr() + "struct " + name + " {\n";

				indentLevel++;
				for (auto& i : _varIndices)
					s += genIndentStr() + std::to_string(*_vars[i.second]->typeName) + " " + i.first + ";\n";
				indentLevel--;

				s += genIndentStr() + "}";

				return s;
			}
		};

		class Scope final : public IStringifiable {
		public:
			std::weak_ptr<Scope> parent;
			std::unordered_map<std::string, std::shared_ptr<FnDef>> fnDefs;
			std::unordered_map<std::string, std::shared_ptr<ImportItem>> imports;
			std::unordered_map<std::string, std::shared_ptr<Type>> types;
			std::unordered_map<std::string, std::shared_ptr<VarDefItem>> vars;

			inline Scope(std::shared_ptr<Scope> parent = std::shared_ptr<Scope>()) {
				this->parent = std::weak_ptr<Scope>(parent);
			}
			inline ~Scope() {
			}

			virtual inline std::string toString() const override {
				std::string s;
				if (imports.size()) {
					s += "import {\n";
					for (auto& i : imports)
						s += "\t" + i.first + " = " + std::to_string(*(i.second->path)) + ",\n";
					s += "}";
				}
				for (auto& i : fnDefs)
					s += std::to_string(*(i.second)) + "\n";
				for (auto& i : types)
					s += std::to_string(*(i.second)) + "\n";

				return s;
			}

			inline std::shared_ptr<FnDef> getFn(std::string name) {
				// Try to get member from local types.
				if (!fnDefs.count(name)) {
					if (types.count(name)) {
						if (!parent.expired())
							return parent.lock()->getFn(name);
					}
					return std::shared_ptr<FnDef>();
				}
				return fnDefs[name];
			}

			static inline std::shared_ptr<Type> getCustomType(std::shared_ptr<TypeName> typeName) {
				if (typeName->typeName != EvalType::CUSTOM)
					return std::shared_ptr<Type>();
				auto tn = std::static_pointer_cast<CustomTypeName>(typeName);
				return tn->scope.lock()->getType(tn->typeRef);
			}

			inline std::shared_ptr<Type> getType(std::shared_ptr<RefExpr> ref) {
				if (types.count(ref->name)) {
					if (ref->next) {
						auto& type = types[ref->name];
						switch (type->getKind()) {
							case Type::Kind::CLASS:
								return std::static_pointer_cast<ClassType>(types[ref->name])->scope->getType(ref->next);
							case Type::Kind::TRAIT:
								return std::static_pointer_cast<TraitType>(types[ref->name])->scope->getType(ref->next);
							default:
								return std::shared_ptr<Type>();
						}
					}
					return types[ref->name];
				}
				if (!parent.expired())
					return parent.lock()->getType(ref);
				return std::shared_ptr<Type>();
			}

			inline std::shared_ptr<Type> getType(std::string name) {
				if (!types.count(name)) {
					if (!parent.expired())
						return parent.lock()->getType(name);
					return std::shared_ptr<Type>();
				}
				return types[name];
			}

			inline std::shared_ptr<VarDefItem> getVar(std::string name) {
				// Try to get member from local types.
				if (!vars.count(name))
					return std::shared_ptr<VarDefItem>();
				return vars[name];
			}
			inline std::shared_ptr<Expr> getEnumItem(std::shared_ptr<RefExpr> ref) {
				if (ref->next) {
					if (!types.count(ref->name))
						return std::shared_ptr<Expr>();
					auto& type = types[ref->name];

					// Check if the next name is terminal
					if (!ref->next->next) {
						// Check if the type is an enumeration
						if (type->getKind() != Type::Kind::ENUM)
							return std::shared_ptr<Expr>();

						auto enumType = std::static_pointer_cast<EnumType>(type);
						if (!enumType->pairs.count(ref->next->name))
							return std::shared_ptr<Expr>();
						return enumType->pairs[ref->next->name];
					}

					switch (type->getKind()) {
						case Type::Kind::CLASS:
							return std::static_pointer_cast<ClassType>(types[ref->name])->scope->getEnumItem(ref->next);
						case Type::Kind::TRAIT:
							return std::static_pointer_cast<TraitType>(types[ref->name])->scope->getEnumItem(ref->next);
						default:
							return std::shared_ptr<Expr>();
					}
				}
				return std::shared_ptr<Expr>();
			}

			void defineVars(std::shared_ptr<VarDefStmt> varDecls);
		};

		extern std::shared_ptr<Scope> currentScope;
		extern std::shared_ptr<EnumType> currentEnum;
		extern std::shared_ptr<ClassType> currentClass;
		extern std::shared_ptr<TraitType> currentTrait;
		extern std::shared_ptr<StructType> currentStruct;
	}
}

#endif
