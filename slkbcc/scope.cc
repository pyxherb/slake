#include "scope.h"

using namespace Slake::Assembler;

shared_ptr<Scope> Slake::Assembler::rootScope, Slake::Assembler::curScope;
shared_ptr<Ref> Slake::Assembler::moduleName;
