#include "scope.hh"

using namespace SpkC::Syntax;

std::shared_ptr<Scope> SpkC::Syntax::currentScope;
std::shared_ptr<Enum> SpkC::Syntax::currentEnum;
std::shared_ptr<Class> SpkC::Syntax::currentClass;
