#ifndef _SPKC_SYNTAX_STMT_HH
#define _SPKC_SYNTAX_STMT_HH

#include <map>

#include "ins.hh"

namespace SpkC {
	namespace Syntax {
		class Scope;
		class Enum;
		class Class;

		extern std::shared_ptr<Scope> currentScope;
		extern std::shared_ptr<Enum> currentEnum;
		extern std::shared_ptr<Class> currentClass;

		class ParamDecl final : public IToken {
		public:
			std::string name;
			std::shared_ptr<TypeName> typeName;
			std::shared_ptr<Expr> initValue;

			ParamDecl(std::string name, std::shared_ptr<TypeName> typeName, std::shared_ptr<Expr> initValue = std::shared_ptr<Expr>()) {
				this->name = name;
				this->typeName = typeName;
				this->initValue = initValue;
			}
			inline ~ParamDecl() {
			}
		};

		struct ParamDeclList final : public IToken {
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
		};

		class FnDef {
		public:
			AccessModifier accessModifier;
			std::shared_ptr<ParamDeclList> params;
			std::shared_ptr<TypeName> returnTypeName;
			std::shared_ptr<CodeBlock> execBlock;

			inline FnDef(
				AccessModifier accessModifier,
				std::shared_ptr<ParamDeclList> params,
				std::shared_ptr<TypeName> returnTypeName,
				std::shared_ptr<CodeBlock> execBlock = std::shared_ptr<CodeBlock>()) {
				this->accessModifier = accessModifier;
				this->params = params;
				this->returnTypeName = returnTypeName;
				this->execBlock = execBlock;
			}
		};

		struct ImportItem final {
		public:
			std::string path;
			bool searchInSystemPath;

			inline ImportItem(std::string path, bool searchInSystemPath = false) {
				this->path = path;
				this->searchInSystemPath = searchInSystemPath;
			}
			inline ~ImportItem() {
			}
		};

		struct VarDefItem final {
			std::shared_ptr<TypeName> typeName;
			Expr* initValue;

			inline VarDefItem(std::shared_ptr<TypeName> typeName, Expr* initValue) {
				this->typeName = typeName;
				this->initValue = initValue;
			}
		};

		class Enum final {
		public:
			AccessModifier accessModifier;
			std::shared_ptr<TypeName> typeName;
			std::map<std::string, std::shared_ptr<Expr>> pairs;

			inline Enum(AccessModifier accessModifier, std::shared_ptr<TypeName> typeName) {
				this->accessModifier = accessModifier;
				this->typeName = typeName;
			}
		};

		struct ImplList final : IToken {
			std::vector<std::shared_ptr<TypeName>> impls;
		};

		class Class final {
		public:
			AccessModifier accessModifier;
			std::shared_ptr<Scope> scope;
			std::shared_ptr<TypeName> parent;
			std::shared_ptr<ImplList> impls;

			inline Class(
				AccessModifier accessModifier,
				std::shared_ptr<Scope> scope,
				std::shared_ptr<TypeName> parent,
				std::shared_ptr<ImplList> impls = std::shared_ptr<ImplList>()
			) {
				this->accessModifier = accessModifier;
				this->scope = scope;
				this->parent = parent;
				this->impls = impls;
			}
		};

		extern int refCount;
		class Scope final {
		public:
			std::shared_ptr<Scope> parent;
			std::map<std::string, std::shared_ptr<FnDef>> fnDefs;
			std::map<std::string, std::shared_ptr<ImportItem>> imports;
			std::map<std::string, std::shared_ptr<Class>> classes, interfaces;
			std::map<std::string, std::shared_ptr<Enum>> enums;
			std::map<std::string, std::shared_ptr<VarDefItem>> vars;

			inline std::shared_ptr<ImportItem> getImport(std::string name) {
				if (imports.count(name))
					return imports[name];
				return std::shared_ptr<ImportItem>();
			}
			inline std::shared_ptr<FnDef> getFn(std::string name) {
				if (fnDefs.count(name))
					return fnDefs[name];
				return std::shared_ptr<FnDef>();
			}
			inline std::shared_ptr<Class> getClass(std::string name) {
				if (classes.count(name))
					return classes[name];
				return std::shared_ptr<Class>();
			}
			inline std::shared_ptr<Enum> getEnum(std::string name) {
				if (enums.count(name))
					return enums[name];
				return std::shared_ptr<Enum>();
			}

			inline Scope(std::shared_ptr<Scope> parent = std::shared_ptr<Scope>()) {
				refCount++;
				this->parent = parent;
			}
			inline ~Scope() {
				refCount--;
			}
		};

		inline void deinit() {
			currentClass.reset();
			currentEnum.reset();
			currentScope.reset();
		}
	}
}

#endif
