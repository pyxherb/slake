#include "scope.hh"

#include <slkparse.hh>

using namespace Slake::Compiler;

std::shared_ptr<Scope> Slake::Compiler::currentScope;
std::shared_ptr<EnumType> Slake::Compiler::currentEnum;
std::shared_ptr<ClassType> Slake::Compiler::currentClass;
std::shared_ptr<TraitType> Slake::Compiler::currentTrait;
std::shared_ptr<StructType> Slake::Compiler::currentStruct;

int Slake::Compiler::indentLevel = 0;

void Slake::Compiler::Scope::defineVars(std::shared_ptr<VarDefStmt> varDecls) {
	for (auto i : varDecls->declList) {
		if (vars.count(i->name))
			throw parser::syntax_error(i->getLocation(), "Redefinition of variable `" + i->name + "'");
		if (varDecls->isNative)
			if ((!(i->initValue)) && (varDecls->accessModifier & ACCESS_STATIC))
				throw parser::syntax_error(i->getLocation(), "UUID for Static native variables is required");
		vars[i->name] = std::make_shared<VarDefItem>(i->getLocation(), varDecls->accessModifier, varDecls->typeName, i->initValue);
	}
}

void Slake::Compiler::StructType::addMembers(std::shared_ptr<VarDefStmt> varDecls) {
	if (varDecls->accessModifier & ~(ACCESS_PUB))
		throw parser::syntax_error(varDecls->getLocation(), "Invalid modifier combination");
	for (auto &i : varDecls->declList) {
		if (varIndices.count(i->name))
			throw parser::syntax_error(varDecls->getLocation(), "Redefinition of member `" + i->name + "'");
		vars.push_back(std::make_shared<VarDefItem>(i->getLocation(), ACCESS_PUB, varDecls->typeName, i->initValue));
		varIndices[i->name] = vars.size() - 1;
	}
}

void Slake::Compiler::deinit() {
	currentTrait.reset();
	currentClass.reset();
	currentEnum.reset();
	currentScope.reset();
	currentStruct.reset();
}
