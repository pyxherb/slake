#include "value.h"

std::uint32_t Slake::Object::_allocCounter = 0;
std::unordered_set<Slake::Object*> Slake::_objectPool;
