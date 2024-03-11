#include <slake/lib/core.h>

using namespace slake;
using namespace slake::corelib;

ModuleValue *corelib::modCore;

ModuleValue *corelib::except::modExcept;
InterfaceValue *corelib::except::typeIException;
ClassValue *corelib::except::exLogicalError;
ClassValue *corelib::except::exDivideByZeroError;
ClassValue *corelib::except::exOutOfMemoryError;
ClassValue *corelib::except::exInvalidOpcodeError;
ClassValue *corelib::except::exInvalidOperandsError;

static ValueRef<> _exceptionConstructor(Runtime *rt, std::deque<Value *> args) {
	if (args.size() != 1)
		throw InvalidArgumentsError("Invalid arguments");
	((VarValue *)memberOf(rt->getActiveContext()->getCurFrame().thisObject, "_msg"))->setData(args[0]);
	return {};
}

static void _initExceptionClass(Runtime *rt, ClassValue *ex) {
	using namespace except;

	ex->implInterfaces.push_back(Type(TypeId::INTERFACE, typeIException));
	ex->scope->addMember("_msg", new VarValue(rt, 0, TypeId::STRING));
	ex->scope->addMember(
		"new",
		new NativeFnValue(
			rt,
			_exceptionConstructor,
			ACCESS_PUB,
			TypeId::NONE));
}

void corelib::except::load(Runtime *rt) {
	modCore->scope->addMember("except",
		modExcept = new ModuleValue(rt, ACCESS_PUB));

	// Set up module `except'
	{
		// Set up interface `Iexception'
		modExcept->scope->addMember("Iexception",
			typeIException = new InterfaceValue(rt, ACCESS_PUB));
		{
			typeIException->scope->addMember("operator@string",
				new FnValue(rt, 0, ACCESS_PUB, TypeId::STRING));
		}

		// Set up exception `LogicalError'
		modExcept->scope->addMember("LogicalError",
			exLogicalError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exLogicalError);

		// Set up exception `DivideByZeroError'
		modExcept->scope->addMember("DivideByZeroError",
			exDivideByZeroError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exDivideByZeroError);

		// Set up exception `OutOfMemoryError'
		modExcept->scope->addMember("OutOfMemoryError",
			exOutOfMemoryError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exOutOfMemoryError);

		// Set up exception `InvalidOpcodeError'
		modExcept->scope->addMember("InvalidOpcodeError",
			exInvalidOpcodeError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exInvalidOpcodeError);

		// Set up exception `InvalidOperandsError'
		modExcept->scope->addMember("InvalidOperandsError",
			exInvalidOperandsError = new ClassValue(rt, ACCESS_PUB));
		_initExceptionClass(rt, exInvalidOperandsError);
	}
}

void corelib::load(Runtime *rt) {
	auto root = rt->getRootValue();

	root->scope->addMember(
		"core",
		corelib::modCore = new ModuleValue(rt, ACCESS_PUB));
	except::load(rt);
}
