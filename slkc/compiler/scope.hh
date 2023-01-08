#ifndef _SLKC_COMPILER_SCOPE_HH
#define _SLKC_COMPILER_SCOPE_HH

#include <map>

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
			inline ~ParamDecl() {
			}

			virtual inline std::string toString() const override {
				std::string s = std::to_string(*typeName) + " " + name;
				if (initValue)
					s += " = " + std::to_string(*initValue);
				return s;
			}
		};

		struct ParamDeclList final : public ILocated,
									 public IStringifiable {
			std::vector<std::shared_ptr<ParamDecl>> decls;

			inline ParamDeclList() {}
			inline ~ParamDeclList() {
			}

			std::shared_ptr<ParamDecl> operator[](std::string name) {
				for (auto& i : decls) {
					if (i->name == name)
						return i;
				}
				return std::shared_ptr<ParamDecl>();
			}

			virtual location getLocation() const override { return decls[0]->getLocation(); }

			virtual inline std::string toString() const override {
				std::string s;

				auto i = decls.begin();
				if (i != decls.end())
					s = std::to_string((**i++));
				while (i != decls.end())
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
			std::string name;

			inline FnDef(
				location loc,
				AccessModifier accessModifier,
				std::shared_ptr<ParamDeclList> params,
				std::shared_ptr<TypeName> returnTypeName,
				std::shared_ptr<CodeBlock> execBlock = std::shared_ptr<CodeBlock>(),
				std::string name = "(Anonymous)") : BasicLocated(loc) {
				this->accessModifier = accessModifier;
				this->params = params;
				this->returnTypeName = returnTypeName;
				this->execBlock = execBlock;
				this->name = name;
			}
			virtual inline ~FnDef() {}

			virtual inline std::string toString() const override {
				std::string s = (returnTypeName ? std::to_string(*returnTypeName) + " " : "") + name + "(" + std::to_string(*params) + ")";
				if (execBlock)
					s += "{\n" + std::to_string(*execBlock) + "}";
				else
					s += ";\n";
				return s;
			}
		};

		struct ImportItem final : public BasicLocated {
		public:
			std::string path;
			bool searchInSystemPath;

			inline ImportItem(location loc, std::string path, bool searchInSystemPath = false) : BasicLocated(loc) {
				this->path = path;
				this->searchInSystemPath = searchInSystemPath;
			}
			inline ~ImportItem() {
			}
		};

		struct VarDefItem final : public BasicLocated {
			std::shared_ptr<TypeName> typeName;
			Expr* initValue;

			inline VarDefItem(location loc, std::shared_ptr<TypeName> typeName, Expr* initValue) : BasicLocated(loc) {
				this->typeName = typeName;
				this->initValue = initValue;
			}
		};

		struct ImplList final : public ILocated,
								public IStringifiable {
			std::vector<std::shared_ptr<TypeName>> impls;

			virtual location getLocation() const override { return impls[0]->getLocation(); }

			virtual inline std::string toString() const override {
				std::string s;

				auto i = impls.begin();
				s = std::to_string((**i));
				while (i != impls.end())
					s += ", " + std::to_string((**i));

				s += " }";
				return s;
			}
		};

		class Type : public IAccessModified,
					 public IStringifiable {
		public:
			enum Kind {
				CLASS = 0,
				INTERFACE,
				ENUM
			};

			inline Type(AccessModifier accessModifier) { this->accessModifier = accessModifier; }
			virtual inline ~Type() {}

			virtual Kind getKind() = 0;
		};

		template <int kind>
		class BasicClassType final : public Type {
		public:
			std::shared_ptr<Scope> scope;
			std::shared_ptr<TypeName> parent;
			std::shared_ptr<ImplList> impls;
			std::string name;

			inline BasicClassType(
				AccessModifier accessModifier,
				std::shared_ptr<Scope> scope,
				std::shared_ptr<TypeName> parent,
				std::string name = "(Anonymous)",
				std::shared_ptr<ImplList> impls = std::shared_ptr<ImplList>()) : Type(accessModifier) {
				this->scope = scope;
				this->parent = parent;
				this->impls = impls;
				this->name = name;
			}
			virtual inline ~BasicClassType() {}

			virtual Kind getKind() {
				return (Kind)kind;
			}

			virtual inline std::string toString() const override {
				std::string s;

				switch (kind) {
					case Kind::CLASS:
						s += "class ";
						break;
					case Kind::INTERFACE:
						s += "interface ";
						break;
					default:
						s += "(classtype) ";
				}

				s += name;

				if (parent)
					s += "(" + std::to_string(*parent) + ")";
				if (impls)
					s += " : " + std::to_string(*impls);

				s += " {\n" + std::to_string(*scope) + "}";

				return s;
			}
		};

		using ClassType = BasicClassType<Type::Kind::CLASS>;
		using InterfaceType = BasicClassType<Type::Kind::INTERFACE>;

		class EnumType final : public Type {
		public:
			std::shared_ptr<TypeName> typeName;
			std::map<std::string, std::shared_ptr<Expr>> pairs;
			std::string name;

			inline EnumType(AccessModifier accessModifier, std::string name, std::shared_ptr<TypeName> typeName) : Type(accessModifier) {
				this->typeName = typeName;
				this->name = name;
			}
			virtual inline ~EnumType() {}

			virtual Kind getKind() {
				return Kind::ENUM;
			}

			virtual inline std::string toString() const override {
				std::string s = "enum : " + std::to_string(*typeName) + " {\n";

				for (auto i : pairs)
					s += "\t" + i.first + " = " + std::to_string(*(i.second)) + ",\n";

				s += " }";

				return s;
			}
		};

		class Scope final : public IStringifiable {
		public:
			std::weak_ptr<Scope> parent;
			std::map<std::string, std::shared_ptr<FnDef>> fnDefs;
			std::map<std::string, std::shared_ptr<ImportItem>> imports;
			std::map<std::string, std::shared_ptr<Type>> types;
			std::map<std::string, std::shared_ptr<VarDefItem>> vars;

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
						s += "\t" + i.first + " = " + (i.second->searchInSystemPath ? "@" : "") + i.second->path + ",\n";
					s += "}";
				}
				for (auto& i : fnDefs)
					s += "\n" + std::to_string(*(i.second));
				for (auto& i : types)
					s += "\n" + std::to_string(*(i.second));

				return s;
			}

			inline std::shared_ptr<FnDef> getFn(std::shared_ptr<RefExpr> ref) {
				if (ref->next || (!fnDefs.count(ref->name))) {
					auto& type = types[ref->name];
					switch (type->getKind()) {
						case Type::Kind::CLASS:
							return std::static_pointer_cast<ClassType>(types[ref->name])->scope->getFn(ref->next);
						case Type::Kind::INTERFACE:
							return std::static_pointer_cast<InterfaceType>(types[ref->name])->scope->getFn(ref->next);
					}
					return std::shared_ptr<FnDef>();
				}
				return fnDefs[ref->name];
			}

			inline std::shared_ptr<Type> getType(std::shared_ptr<RefExpr> ref) {
				if (types.count(ref->name)) {
					if (ref->next) {
						auto& type = types[ref->name];
						switch (type->getKind()) {
							case Type::Kind::CLASS:
								return std::static_pointer_cast<ClassType>(types[ref->name])->scope->getType(ref->next);
							case Type::Kind::INTERFACE:
								return std::static_pointer_cast<InterfaceType>(types[ref->name])->scope->getType(ref->next);
						}
						return std::shared_ptr<Type>();
					}
					return types[ref->name];
				}
				return std::shared_ptr<Type>();
			}

			inline std::shared_ptr<VarDefItem> getVar(std::shared_ptr<RefExpr> ref) {
				if (ref->next || (!vars.count(ref->name))) {
					auto& type = types[ref->name];
					switch (type->getKind()) {
						case Type::Kind::CLASS:
							return std::static_pointer_cast<ClassType>(types[ref->name])->scope->getVar(ref->next);
						case Type::Kind::INTERFACE:
							return std::static_pointer_cast<InterfaceType>(types[ref->name])->scope->getVar(ref->next);
					}
					return std::shared_ptr<VarDefItem>();
				}
				return vars[ref->name];
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
						case Type::Kind::INTERFACE:
							return std::static_pointer_cast<InterfaceType>(types[ref->name])->scope->getEnumItem(ref->next);
					}
				}
				return std::shared_ptr<Expr>();
			}
		};

		extern std::shared_ptr<Scope> currentScope;
		extern std::shared_ptr<EnumType> currentEnum;
		extern std::shared_ptr<ClassType> currentClass;
		extern std::shared_ptr<InterfaceType> currentInterface;
	}
}

#endif
