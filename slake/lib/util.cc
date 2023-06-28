#include "util.h"
#include "std.h"
#include <thread>

using namespace Slake;
using namespace Slake::StdLib;
using namespace Slake::StdLib::Util;

ModuleValue *StdLib::Util::modUtil = nullptr;
ModuleValue *StdLib::Util::Math::modMath = nullptr;

#define PI 3.141592653589793

static double _sin(double x) {
	if (x == NAN || x == INFINITY)
		return x;

	double sum = x, lastSum = NAN;
	double factorial = 1;  // n!
	double power = x;	   // x ^ (2n + 1)

	for (unsigned int i = 1; lastSum != sum; ++i) {
		lastSum = sum;

		factorial *= (i << 1) * ((i << 1) + 1);
		power *= x * x;

		sum += ((i & 1 ? -1.0f : 1.0f) / (factorial)) * power;
	}

	return sum;
}

#define _cos(x) (_sin(PI / 2 - (x)))
#define _tan(x) (_sin(x) / _cos(x))

ValueRef<> StdLib::Util::Math::sin(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args) {
	if (nArgs != 1)
		throw InvalidArgumentsError();

	Value* x = *args[0];

	if (!x)
		throw NullRefError();
	if (x->getType() != ValueType::F32)
		throw InvalidArgumentsError();

	return new F32Value(rt, _sin(((F32Value *)x)->getValue()));
}

ValueRef<> StdLib::Util::Math::cos(Slake::Runtime *rt, uint8_t nArgs, Slake::ValueRef<> *args) {
	if (nArgs != 1)
		throw InvalidArgumentsError();

	Value *x = *args[0];

	if (!x)
		throw NullRefError();
	if (x->getType() != ValueType::F32)
		throw InvalidArgumentsError();

	return new F32Value(rt, _cos(((F32Value *)x)->getValue()));
}

void StdLib::Util::Math::load(Runtime *rt) {
	modUtil->addMember("math",
		modMath = new ModuleValue(rt, ACCESS_PUB));

	modMath->addMember("sin", new NativeFnValue(rt, Math::sin, ACCESS_PUB, ValueType::F32));
	modMath->addMember("cos", new NativeFnValue(rt, Math::cos, ACCESS_PUB, ValueType::F32));
}

void StdLib::Util::load(Runtime *rt) {
	auto root = rt->getRootValue();

	modStd->addMember(
		"util",
		modUtil= new ModuleValue(rt, ACCESS_PUB));
	Math::load(rt);
}
