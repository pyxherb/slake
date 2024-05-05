#include <slake/lib/std.h>
#include <slake/lib/std/math.h>

#include <cmath>
#include <thread>

#ifdef __GNUC__
	#ifdef __amd64__
extern "C" {
float _slake_stdlib_sinf_fast(float x);
double _slake_stdlib_sin_fast(double x);
float _slake_stdlib_cosf_fast(float x);
double _slake_stdlib_cos_fast(double x);
float _slake_stdlib_tanf_fast(float x);
double _slake_stdlib_tan_fast(double x);
float _slake_stdlib_sqrtf_fast(float x);
double _slake_stdlib_sqrt_fast(double x);
}
		#define _sinf_fast _slake_stdlib_sinf_fast
		#define _sin_fast _slake_stdlib_sin_fast
		#define _cosf_fast _slake_stdlib_cosf_fast
		#define _cos_fast _slake_stdlib_cos_fast
		#define _tanf_fast _slake_stdlib_tanf_fast
		#define _tan_fast _slake_stdlib_tan_fast
		#define _sqrtf_fast _slake_stdlib_sqrtf_fast
		#define _sqrt_fast _slake_stdlib_sqrt_fast
	#endif
#endif

#ifndef _sin_fast
	#define _sin_fast(x) sin(x)
#endif

#ifndef _sinf_fast
	#define _sinf_fast(x) sinf(x)
#endif

#ifndef _cos_fast
	#define _cos_fast(x) cos(x)
#endif

#ifndef _cosf_fast
	#define _cosf_fast(x) cosf(x)
#endif

#ifndef _tan_fast
	#define _tan_fast(x) tan(x)
#endif

#ifndef _tanf_fast
	#define _tanf_fast(x) tanf(x)
#endif

#ifndef _sqrt_fast
	#define _sqrt_fast(x) sqrt(x)
#endif

#ifndef _sqrtf_fast
	#define _sqrtf_fast(x) sqrtf(x)
#endif

using namespace slake;
using namespace slake::stdlib;
using namespace slake::stdlib::math;

ModuleValue *stdlib::math::modMath = nullptr;

#define PI 3.141592653589793

static double _sin(double x);
static double _cos(double x);
static double _tan(double x);
static double _sqrt(double x);

//
// Native function callback implementations start
//

#define _nArgCheck(op, n) \
	if (!(args.size() op(n))) throw InvalidArgumentsError()
#define _nullRefCheck(x) \
	if (!(x)) throw NullRefError()
#define _typeCheck(x, t) \
	if ((x)->getType() != (t)) throw InvalidArgumentsError()

template <typename T>
static ValueRef<> _sinImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	using ValueType = LiteralValue<T, getValueType<T>()>;
	_nArgCheck(==, 1);

	Value *x = args[0];

	_nullRefCheck(x);
	_typeCheck(x, getValueType<T>());

	return new ValueType(rt, (T)_sin(((ValueType *)x)->getData()));
}

template <typename T>
static ValueRef<> _cosImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	using ValueType = LiteralValue<T, getValueType<T>()>;
	_nArgCheck(==, 1);

	Value *x = args[0];

	_nullRefCheck(x);
	_typeCheck(x, getValueType<T>());

	return new ValueType(rt, (T)_cos(((ValueType *)x)->getData()));
}

template <typename T>
static ValueRef<> _tanImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	using ValueType = LiteralValue<T, getValueType<T>()>;
	_nArgCheck(==, 1);

	Value *x = args[0];

	_nullRefCheck(x);
	_typeCheck(x, getValueType<T>());

	return new ValueType(rt, (T)_tan(((ValueType *)x)->getData()));
}

template <typename T>
static ValueRef<> _sqrtImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	using ValueType = LiteralValue<T, getValueType<T>()>;
	_nArgCheck(==, 1);

	Value *x = args[0];

	_nullRefCheck(x);
	_typeCheck(x, getValueType<T>());

	return new ValueType(rt, (T)_sqrt(((ValueType *)x)->getData()));
}

static ValueRef<> _sinFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F64Value *x = (F64Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F64);

	return new F64Value(rt, _sin_fast(x->getData()));
}

static ValueRef<> _sinfFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F32Value *x = (F32Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F32);

	return new F32Value(rt, _sinf_fast(x->getData()));
}

static ValueRef<> _cosFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F64Value *x = (F64Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F64);

	return new F64Value(rt, _cos_fast(x->getData()));
}

static ValueRef<> _cosfFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F32Value *x = (F32Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F32);

	return new F32Value(rt, _cosf_fast(x->getData()));
}

static ValueRef<> _tanFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F64Value *x = (F64Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F64);

	return new F64Value(rt, _tan_fast(x->getData()));
}

static ValueRef<> _tanfFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F32Value *x = (F32Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F32);

	return new F32Value(rt, _tanf_fast(x->getData()));
}

static ValueRef<> _sqrtFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F64Value *x = (F64Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F64);

	return new F64Value(rt, _sqrt_fast(x->getData()));
}

static ValueRef<> _sqrtfFastImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	_nArgCheck(==, 1);

	F32Value *x = (F32Value *)args[0];

	_nullRefCheck(x);
	_typeCheck(x, TypeId::F32);

	return new F32Value(rt, _sqrtf_fast(x->getData()));
}

template <typename T>
static ValueRef<> _absImpl(
	Runtime *rt,
	Value *thisObject,
	std::deque<Value *> args,
	const std::unordered_map<std::string, Type> &mappedGenericArgs) {
	using ValueType = LiteralValue<T, getValueType<T>()>;
	_nArgCheck(==, 1);

	Value *x = args[0];

	_nullRefCheck(x);
	_typeCheck(x, getValueType<T>());

	auto n = ((ValueType *)x)->getData();

	if constexpr (std::is_same<T, float>::value) {
		*(uint32_t *)&n &= ~0x800000;
	} else if constexpr (std::is_same<T, double>::value) {
		*(uint32_t *)&n &= ~0x8000000000000000LL;
	} else if constexpr (std::is_integral<T>::value && std::is_signed<T>::value) {
		n = ~n + 1;
	}

	return new ValueType(rt, n);
}

//
// Native function callback implementations end
//

void stdlib::math::load(Runtime *rt) {
	modStd->scope->addMember("math",
		modMath = new ModuleValue(rt, ACCESS_PUB));

	modMath->scope->addMember(
		rt->mangleName("sin", { TypeId::F32 }),
		new NativeFnValue(rt, _sinImpl<float>, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("sin", { TypeId::F64 }),
		new NativeFnValue(rt, _sinImpl<double>, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("sinFast", { TypeId::F32 }),
		new NativeFnValue(rt, _sinfFastImpl, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("sinFast", { TypeId::F64 }),
		new NativeFnValue(rt, _sinFastImpl, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("cos", { TypeId::F32 }),
		new NativeFnValue(rt, _cosImpl<float>, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("cos", { TypeId::F64 }),
		new NativeFnValue(rt, _cosImpl<double>, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("cosFast", { TypeId::F32 }),
		new NativeFnValue(rt, _cosfFastImpl, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("cosFast", { TypeId::F64 }),
		new NativeFnValue(rt, _cosFastImpl, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("tan", { TypeId::F32 }),
		new NativeFnValue(rt, _tanImpl<float>, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("tan", { TypeId::F64 }),
		new NativeFnValue(rt, _tanImpl<double>, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("tanFast", { TypeId::F32 }),
		new NativeFnValue(rt, _tanfFastImpl, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("tanFast", { TypeId::F64 }),
		new NativeFnValue(rt, _tanFastImpl, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("sqrt", { TypeId::F32 }),
		new NativeFnValue(rt, _sqrtImpl<float>, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("sqrt", { TypeId::F64 }),
		new NativeFnValue(rt, _sqrtImpl<double>, ACCESS_PUB, TypeId::F64));

	modMath->scope->addMember(
		rt->mangleName("sqrtFast", { TypeId::F32 }),
		new NativeFnValue(rt, _sqrtfFastImpl, ACCESS_PUB, TypeId::F32));
	modMath->scope->addMember(
		rt->mangleName("sqrtFast", { TypeId::F64 }),
		new NativeFnValue(rt, _sqrtFastImpl, ACCESS_PUB, TypeId::F64));
}

static double _sin(double x) {
	if (std::isnan(x) || std::isinf(x))
		return x;

	x = fmod(x, 3.14159265358979323846 * 2);

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

static double _cos(double x) { return _sin(PI / 2 - x); }
static double _tan(double x) { return _sin(x) / _cos(x); }

static double _sqrt(double x) {
	if (std::isnan(x) || std::isinf(x))
		return x;

	if (x < 0.0)
		return NAN;

	double sum = x, lastSum;

	do {
		lastSum = sum;

		// sum[i+1] = (sum[i] + x/sum[i]) / n
		sum = (sum + x / sum) * 0.5;
	} while (sum != lastSum);

	return sum;
}

static double _cbrt(double a) {
	double result = a, lastResult;

	do {
		lastResult = result;

		// f(x) = x^3 - a = 0
		// f'(x) = 3x^2
		// x -= (x^3-a)/(3x^2)^2
		// x -= (x - a/x/x) / 81
		// x -= (x - a/x/x) * (1 / 81)
		result -= (result - a / (result * result)) * 0.012345679012345678;
	} while (result != lastResult);

	return result;
}
