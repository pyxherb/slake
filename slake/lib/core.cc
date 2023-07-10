#include "core.h"

#include "std.h"

using namespace slake;
using namespace slake::stdlib::Core;

ModuleValue *stdlib::Core::modCore;

ModuleValue *stdlib::Core::Except::modExcept;
InterfaceValue *stdlib::Core::Except::typeIException;
ClassValue *stdlib::Core::Except::exLogicalError;
ClassValue *stdlib::Core::Except::exDivideByZeroError;
ClassValue *stdlib::Core::Except::exOutOfMemoryError;
ClassValue *stdlib::Core::Except::exInvalidOpcodeError;
ClassValue *stdlib::Core::Except::exInvalidOperandsError;

static ValueRef<> _exceptionConstructor(Runtime *rt, uint8_t nArgs, ValueRef<> *args) {
	if (nArgs != 1)
		throw InvalidArgumentsError("Invalid arguments");
	((VarValue *)(*rt->getCurContext()->getCurFrame().thisObject)->getMember("_msg"))->setData(*args[0]);
	return {};
}

static void _initExceptionClass(Runtime *rt, ClassValue *ex) {
	using namespace Except;

	ex->implInterfaces.push_back(Type(ValueType::INTERFACE, typeIException));
	ex->addMember("_msg", new VarValue(rt, 0, ValueType::STRING));
	ex->addMember(
		"new",
		new NativeFnValue(
			rt,
			_exceptionConstructor,
			ACCESS_PUB,
			ValueType::NONE));
}

void stdlib::Core::Except::load(Runtime *rt) {
	modCore->addMember("except",
		modExcept = new ModuleValue(rt, ACCESS_PUB));

	// Set up module `except'
	{
		// Set up interface `IException'
		modExcept->addMember("IException",
			typeIException = new InterfaceValue(rt, ACCESS_PUB));
		{
			typeIException->addMember("operator@string",
				new FnValue(rt, 0, ACCESS_PUB, ValueType::STRING));
		}

		// Set up exception `LogicalError'
		modExcept->addMember("LogicalError",
			exLogicalError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exLogicalError);

		// Set up exception `DivideByZeroError'
		modExcept->addMember("DivideByZeroError",
			exDivideByZeroError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exDivideByZeroError);

		// Set up exception `OutOfMemoryError'
		modExcept->addMember("OutOfMemoryError",
			exOutOfMemoryError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exOutOfMemoryError);

		// Set up exception `InvalidOpcodeError'
		modExcept->addMember("InvalidOpcodeError",
			exInvalidOpcodeError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exInvalidOpcodeError);

		// Set up exception `InvalidOperandsError'
		modExcept->addMember("InvalidOperandsError",
			exInvalidOperandsError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exInvalidOperandsError);
	}
}

void stdlib::Core::load(Runtime *rt) {
	auto root = rt->getRootValue();

	modStd->addMember(
		"core",
		Core::modCore = new ModuleValue(rt, ACCESS_PUB));
	Except::load(rt);
}
