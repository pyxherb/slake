#include "scope.h"

using namespace slake::bcc;

shared_ptr<Scope> slake::bcc::rootScope, slake::bcc::curScope;
shared_ptr<Ref> slake::bcc::moduleName;
