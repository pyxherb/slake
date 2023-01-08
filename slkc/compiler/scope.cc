#include "scope.hh"

using namespace Slake::Compiler;

std::shared_ptr<Scope> Slake::Compiler::currentScope;
std::shared_ptr<EnumType> Slake::Compiler::currentEnum;
std::shared_ptr<ClassType> Slake::Compiler::currentClass;
std::shared_ptr<InterfaceType> Slake::Compiler::currentInterface;
